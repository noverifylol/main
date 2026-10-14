#pragma once
#include "../vendor/nlohmann/json.hpp"
#include "../vendor/monocypher/monocypher-ed25519.h"
#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <ctime>
#include <stdexcept>
#include <string>
#include <vector>

namespace v6x::licensing {
using Json = nlohmann::json;
inline constexpr const char *signingKey = "5586b4bc69c7a4b487e4563a4cd96afd39140f919bd31cea7d1c6a1e8439422b";
struct Error : std::runtime_error {
    std::string code;
    Error(std::string c, std::string message) : std::runtime_error(std::move(message)), code(std::move(c)) {}
};
inline std::string formEncode(const std::string &text) {
    constexpr char hex[] = "0123456789ABCDEF";
    std::string out;
    for (unsigned char c : text) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' ||
            c == '_' || c == '.' || c == '~')
            out += static_cast<char>(c);
        else {
            out += '%';
            out += hex[c >> 4];
            out += hex[c & 15];
        }
    }
    return out;
}
inline std::vector<std::uint8_t> hexBytes(const std::string &value, size_t size) {
    if (value.size() != size * 2)
        throw Error("signature_format", "The authentication response has an invalid signature.");
    auto nibble = [](char c) -> int {
        if (c >= '0' && c <= '9')
            return c - '0';
        if (c >= 'a' && c <= 'f')
            return c - 'a' + 10;
        if (c >= 'A' && c <= 'F')
            return c - 'A' + 10;
        return -1;
    };
    std::vector<std::uint8_t> out(size);
    for (size_t i = 0; i < size; ++i) {
        int a = nibble(value[2 * i]), b = nibble(value[2 * i + 1]);
        if (a < 0 || b < 0)
            throw Error("signature_format", "The authentication response has an invalid signature.");
        out[i] = static_cast<std::uint8_t>((a << 4) | b);
    }
    return out;
}
inline std::int64_t integer(const Json &value) {
    if (value.is_number_integer())
        return value.get<std::int64_t>();
    if (!value.is_string())
        return 0;
    const auto s = value.get<std::string>();
    std::int64_t n{};
    auto result = std::from_chars(s.data(), s.data() + s.size(), n);
    return result.ec == std::errc{} && result.ptr == s.data() + s.size() ? n : 0;
}
inline void verifyResponse(const std::string &body, const std::string &signature,
                           const std::string &timestamp, std::int64_t now,
                           const std::string &publicKey = signingKey) {
    if (body.size() > 1024 * 1024 || timestamp.size() < 10 || timestamp.size() > 12)
        throw Error("response_format", "The authentication response could not be verified.");
    std::int64_t signedAt{};
    auto r = std::from_chars(timestamp.data(), timestamp.data() + timestamp.size(), signedAt);
    if (r.ec != std::errc{} || r.ptr != timestamp.data() + timestamp.size() || signedAt < 0)
        throw Error("signature_time", "Invalid authentication timestamp.");
    if (signedAt > now + 120 || signedAt < now - 120)
        throw Error("clock_skew", "Check your Windows date and time, then try again.");
    auto sig = hexBytes(signature, 64), key = hexBytes(publicKey, 32);
    std::string message = timestamp + body;
    if (crypto_ed25519_check(sig.data(), key.data(), reinterpret_cast<const std::uint8_t *>(message.data()),
                             message.size()) != 0)
        throw Error("signature_invalid",
                    "The authentication response signature is invalid. Access was not granted.");
}
inline Json verifiedJson(const std::string &body, const std::string &owner) {
    Json obj;
    try {
        obj = Json::parse(body);
    } catch (...) {
        throw Error("response_format", "KeyAuth returned an unreadable response.");
    }
    if (!obj.is_object() || !obj.contains("ownerid") || !obj["ownerid"].is_string() ||
        obj["ownerid"].get<std::string>() != owner)
        throw Error("owner_mismatch",
                    "The authentication response belongs to a different application owner.");
    if (!obj.contains("success") || !obj["success"].is_boolean())
        throw Error("response_format", "Invalid authentication response format.");
    if (obj["success"].get<bool>() &&
        (!obj.contains("code") || !obj["code"].is_number_integer() || obj["code"].get<int>() != 68))
        throw Error("response_code", "Unexpected authentication success code. Access was not granted.");
    return obj;
}
inline std::string serviceError(const Json &obj) {
    std::string text = obj.value("message", std::string{});
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char c) { return static_cast<char>(c >= 'A' && c <= 'Z' ? c + 32 : c); });
    if (text.find("invalidver") != std::string::npos)
        return "This application version is no longer accepted. Contact support.";
    if (text.find("expired") != std::string::npos)
        return "Your license or session has expired. Please sign in again.";
    if (text.find("hwid") != std::string::npos)
        return "This license is linked to a different Windows identity. Contact support.";
    if (text.find("ban") != std::string::npos || text.find("blacklist") != std::string::npos)
        return "This license or account is blocked. Contact support.";
    if (text.find("2fa") != std::string::npos || text.find("two-factor") != std::string::npos)
        return "A valid two-factor code is required.";
    if (text.find("session") != std::string::npos)
        return "Your session is no longer valid. Reconnect and sign in again.";
    if (text.find("key") != std::string::npos || text.find("license") != std::string::npos)
        return "The license was not accepted. Check the key and selected game.";
    if (text.find("pause") != std::string::npos)
        return "This application is currently paused by its owner.";
    if (text.find("name") != std::string::npos || text.find("application") != std::string::npos)
        return "The application's KeyAuth configuration was not accepted.";
    int code = obj.contains("code") && obj["code"].is_number_integer() ? obj["code"].get<int>() : 0;
    return "KeyAuth denied the request (code " + std::to_string(code) + "). Contact support.";
}
inline std::int64_t subscriptionExpiry(const Json &response, std::int64_t now) {
    std::int64_t until = 0;
    try {
        for (const auto &sub : response.at("info").at("subscriptions"))
            if (sub.contains("expiry"))
                until = std::max(until, integer(sub.at("expiry")));
    } catch (...) {
        throw Error("subscription_data", "The server did not return usable subscription information.");
    }
    if (until <= now)
        throw Error("subscription_expired", "No active subscription was returned for this application.");
    return until;
}
} // namespace v6x::licensing
