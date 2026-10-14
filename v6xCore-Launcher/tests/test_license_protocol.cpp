#include "../src/license_protocol.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <functional>
#include <iostream>
using namespace v6x::licensing;
bool rejects(const std::function<void()> &action) {
    try {
        action();
        return false;
    } catch (const std::exception &) {
        return true;
    }
}
std::string hex(const unsigned char *data, size_t size) {
    const char *h = "0123456789abcdef";
    std::string out;
    for (size_t i = 0; i < size; ++i) {
        out += h[data[i] >> 4];
        out += h[data[i] & 15];
    }
    return out;
}
int main() {
    // Synthetic test signing key. It is not the KeyAuth signing key or a customer credential.
    unsigned char seed[32]{}, secret[64]{}, pub[32]{}, signature[64]{};
    seed[0] = 19;
    crypto_ed25519_key_pair(secret, pub, seed);
    std::string stamp = "1800000000";
    Json data = {
        {"ownerid", "test-owner"},
        {"success", true},
        {"code", 68},
        {"sessionid", "synthetic-test-session"},
        {"info",
         {{"subscriptions", Json::array({{{"subscription", "default"}, {"expiry", "1800003600"}}})}}}};
    std::string body = data.dump(), message = stamp + body;
    crypto_ed25519_sign(signature, secret, reinterpret_cast<const uint8_t *>(message.data()), message.size());
    const auto sig = hex(signature, 64), key = hex(pub, 32);
    verifyResponse(body, sig, stamp, 1800000000, key);
    assert(verifiedJson(body, "test-owner")["success"].get<bool>());
    assert(subscriptionExpiry(data, 1800000000) == 1800003600);
    assert(rejects(
        [&] { verifyResponse(body, sig, stamp, 1800000000); })); // Production pin rejects the test signer.
    assert(rejects([&] { verifyResponse(body + " ", sig, stamp, 1800000000, key); }));
    assert(rejects([&] { verifyResponse(body, sig, stamp, 1800000121, key); }));
    assert(rejects([&] { verifyResponse(body, sig, stamp, 1799999879, key); }));
    assert(rejects([&] { verifyResponse(body, "bad", stamp, 1800000000, key); }));
    assert(rejects([&] { verifiedJson(body, "wrong-owner"); }));
    auto invalid = data;
    invalid["code"] = 0;
    assert(rejects([&] { verifiedJson(invalid.dump(), "test-owner"); }));
    invalid = data;
    invalid["success"] = "true";
    assert(rejects([&] { verifiedJson(invalid.dump(), "test-owner"); }));
    assert(rejects([&] { subscriptionExpiry(data, 1800004000); }));
    assert(formEncode("Counter Strike 2") == "Counter%20Strike%202");
    assert(formEncode("a&b=+%") == "a%26b%3D%2B%25");
    Json denied = {{"success", false}, {"message", "Key TEST-PRIVATE-VALUE was not found"}, {"code", 1}};
    assert(serviceError(denied).find("TEST-PRIVATE-VALUE") == std::string::npos);
    std::cout << "PASS: signed-response verification, production-key rejection, tampering, replay window, "
                 "owner/code/type checks, expiry and credential-safe errors\n";
}
