#pragma once
#include "catalog.hpp"
#include "license_protocol.hpp"
#include "updater.hpp"
#include <wincrypt.h>
#include <sddl.h>
#include <optional>

namespace v6x::licensing {
inline void wipe(std::string &s) {
    if (!s.empty())
        SecureZeroMemory(s.data(), s.size());
    s.clear();
}
struct WipeAtEnd {
    std::string &text;
    ~WipeAtEnd() {
        wipe(text);
    }
};
inline std::filesystem::path licenseDirectory() {
    PWSTR path{};
    if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, nullptr, &path)) || !path)
        throw Error("cache_path", "Windows could not locate the license storage folder.");
    auto out = std::filesystem::path(path) / L"v6xCore" / L"licenses";
    CoTaskMemFree(path);
    std::filesystem::create_directories(out);
    return out;
}
class LicenseCache {
    std::filesystem::path root_;
    static std::string binding(const GameDefinition &game) {
        return std::string("v6xCore/license/v1/") + game.id + "/" + game.ownerId + "/" + game.appName;
    }

  public:
    explicit LicenseCache(std::filesystem::path root = {})
        : root_(root.empty() ? licenseDirectory() : std::move(root)) {}
    std::filesystem::path path(const GameDefinition &game) const {
        return root_ / (std::string(game.id) + "-" + v6x::sha256(binding(game)).substr(0, 12) + ".bin");
    }
    bool exists(const GameDefinition &game) const {
        std::error_code e;
        return std::filesystem::is_regular_file(path(game), e);
    }
    void remove(const GameDefinition &game) {
        std::error_code e;
        std::filesystem::remove(path(game), e);
        if (e)
            throw Error("cache_remove", "The saved license could not be removed.");
    }
    void save(const GameDefinition &game, const std::string &license) {
        std::string plain =
            Json{{"application", game.appName}, {"owner", game.ownerId}, {"license", license}}.dump();
        WipeAtEnd clean{plain};
        std::string entropy = binding(game);
        DATA_BLOB input{static_cast<DWORD>(plain.size()), reinterpret_cast<BYTE *>(plain.data())};
        DATA_BLOB salt{static_cast<DWORD>(entropy.size()), reinterpret_cast<BYTE *>(entropy.data())},
            output{};
        if (!CryptProtectData(&input, L"v6xCore license", &salt, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN,
                              &output))
            throw Error(
                "cache_protect",
                "Windows could not protect the license. Sign-in can continue without remembering it.");
        std::string encrypted = "V6XDPAPI1";
        encrypted.append(reinterpret_cast<const char *>(output.pbData), output.cbData);
        LocalFree(output.pbData);
        try {
            atomicWrite(path(game), encrypted);
        } catch (...) {
            throw Error("cache_write", "The encrypted license could not be saved.");
        }
    }
    std::optional<std::string> load(const GameDefinition &game) {
        if (!exists(game))
            return std::nullopt;
        std::string encrypted;
        try {
            encrypted = readBytes(path(game), 65536);
        } catch (...) {
            throw Error("cache_read", "The saved license could not be read.");
        }
        if (encrypted.compare(0, 9, "V6XDPAPI1") != 0)
            throw Error("cache_format", "The saved license file is not recognized.");
        std::string entropy = binding(game);
        DATA_BLOB input{static_cast<DWORD>(encrypted.size() - 9),
                        reinterpret_cast<BYTE *>(encrypted.data() + 9)};
        DATA_BLOB salt{static_cast<DWORD>(entropy.size()), reinterpret_cast<BYTE *>(entropy.data())},
            output{};
        if (!CryptUnprotectData(&input, nullptr, &salt, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output))
            throw Error("cache_unprotect",
                        "This Windows account cannot unlock the saved license. Enter your key again.");
        std::string plain(reinterpret_cast<char *>(output.pbData), output.cbData);
        SecureZeroMemory(output.pbData, output.cbData);
        LocalFree(output.pbData);
        WipeAtEnd clean{plain};
        try {
            auto data = Json::parse(plain);
            if (data.value("application", std::string{}) != game.appName ||
                data.value("owner", std::string{}) != game.ownerId)
                throw std::runtime_error("binding");
            std::string key = data.at("license").get<std::string>();
            if (key.empty() || key.size() > 255)
                throw std::runtime_error("key size");
            return key;
        } catch (...) {
            throw Error("cache_content", "The saved license is invalid. Enter your key again.");
        }
    }
};
inline std::string windowsIdentity(const GameDefinition &game) {
    HANDLE raw{};
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &raw))
        throw Error("windows_identity", "Windows identity information is unavailable.");
    cs2::Handle token(raw);
    DWORD n{};
    GetTokenInformation(token.get(), TokenUser, nullptr, 0, &n);
    if (n == 0 || n > 65536)
        throw Error("windows_identity", "Windows identity information is unavailable.");
    std::vector<unsigned char> data(n);
    if (!GetTokenInformation(token.get(), TokenUser, data.data(), n, &n))
        throw Error("windows_identity", "Windows identity information is unavailable.");
    LPWSTR text{};
    if (!ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER *>(data.data())->User.Sid, &text))
        throw Error("windows_identity", "Windows identity information is unavailable.");
    std::string identity = utf8(text);
    LocalFree(text);
    WipeAtEnd clean{identity};
    return v6x::sha256(std::string("v6xCore/") + game.id + "/" + game.ownerId + "/" + identity);
}
inline std::string executableMd5() {
    std::wstring executable(32768, 0);
    DWORD n = GetModuleFileNameW(nullptr, executable.data(), static_cast<DWORD>(executable.size()));
    if (!n || n >= executable.size())
        throw Error("exe_path", "The current executable could not be identified.");
    executable.resize(n);
    std::string bytes = readBytes(std::filesystem::path(executable), 64 * 1024 * 1024);
    BCRYPT_ALG_HANDLE alg{};
    BCRYPT_HASH_HANDLE hash{};
    std::vector<UCHAR> object;
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_MD5_ALGORITHM, nullptr, 0) < 0)
        throw Error("exe_hash", "The application checksum could not be calculated.");
    try {
        DWORD size{}, got{};
        if (BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&size), sizeof(size), &got,
                              0) < 0 ||
            size > 1024 * 1024)
            throw std::runtime_error("hash");
        object.resize(size);
        if (BCryptCreateHash(alg, &hash, object.data(), size, nullptr, 0, 0) < 0)
            throw std::runtime_error("hash");
        std::array<UCHAR, 16> digest{};
        if (BCryptHashData(hash, reinterpret_cast<PUCHAR>(bytes.data()), static_cast<ULONG>(bytes.size()),
                           0) < 0 ||
            BCryptFinishHash(hash, digest.data(), static_cast<ULONG>(digest.size()), 0) < 0)
            throw std::runtime_error("hash");
        BCryptDestroyHash(hash);
        hash = nullptr;
        BCryptCloseAlgorithmProvider(alg, 0);
        alg = nullptr;
        std::ostringstream out;
        out << std::hex << std::setfill('0');
        for (auto value : digest)
            out << std::setw(2) << static_cast<unsigned>(value);
        return out.str();
    } catch (...) {
        if (hash)
            BCryptDestroyHash(hash);
        if (alg)
            BCryptCloseAlgorithmProvider(alg, 0);
        throw Error("exe_hash", "The application checksum could not be calculated.");
    }
}
struct HttpReply {
    std::string body, signature, timestamp;
};
inline std::string responseHeader(HINTERNET request, const wchar_t *name) {
    DWORD bytes{};
    WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM, name, nullptr, &bytes, WINHTTP_NO_HEADER_INDEX);
    if (!bytes || bytes > 8192)
        return {};
    std::wstring value(bytes / sizeof(wchar_t) + 1, 0);
    if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM, name, value.data(), &bytes,
                             WINHTTP_NO_HEADER_INDEX))
        return {};
    value.resize(wcsnlen(value.c_str(), value.size()));
    return utf8(value);
}
inline HttpReply post(const std::string &data, bool paidCheck, std::atomic<bool> &cancelled) {
    if (cancelled)
        throw Error("cancelled", "Authentication was cancelled.");
    HttpHandle session(WinHttpOpen(L"v6xCore/3.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
                                   WINHTTP_NO_PROXY_BYPASS, 0));
    if (!session)
        throw Error("network_init", "Windows could not initialize HTTPS.");
    WinHttpSetTimeouts(session, 5000, 5000, 10000, 10000);
    HttpHandle connection(WinHttpConnect(session, L"keyauth.win", INTERNET_DEFAULT_HTTPS_PORT, 0));
    if (!connection)
        throw Error("network_connect", "Could not connect to KeyAuth. Check your connection.");
    const wchar_t *path = paidCheck ? L"/api/1.3/?check_paid=1" : L"/api/1.3/";
    HttpHandle request(WinHttpOpenRequest(connection, L"POST", path, nullptr, WINHTTP_NO_REFERER,
                                          WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE));
    if (!request)
        throw Error("network_request", "Could not create the authentication request.");
    DWORD policy = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    WinHttpSetOption(request, WINHTTP_OPTION_REDIRECT_POLICY, &policy, sizeof(policy));
    if (!WinHttpSendRequest(
            request, L"Content-Type: application/x-www-form-urlencoded\r\nAccept: application/json\r\n",
            static_cast<DWORD>(-1), const_cast<char *>(data.data()), static_cast<DWORD>(data.size()),
            static_cast<DWORD>(data.size()), 0) ||
        !WinHttpReceiveResponse(request, nullptr))
        throw Error("network_tls", "The HTTPS request failed. Check your connection and Windows clock.");
    DWORD status{}, size = sizeof(status);
    if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX) ||
        status != 200)
        throw Error("http_status",
                    "The authentication service is unavailable (HTTP " + std::to_string(status) + ").");
    HttpReply reply;
    reply.signature = responseHeader(request, L"x-signature-ed25519");
    reply.timestamp = responseHeader(request, L"x-signature-timestamp");
    std::array<char, 16384> buffer{};
    for (;;) {
        if (cancelled)
            throw Error("cancelled", "Authentication was cancelled.");
        DWORD got{};
        if (!WinHttpReadData(request, buffer.data(), static_cast<DWORD>(buffer.size()), &got))
            throw Error("network_read", "The authentication response was interrupted.");
        if (!got)
            break;
        if (reply.body.size() + got > 1024 * 1024)
            throw Error("response_size", "The authentication response was too large.");
        reply.body.append(buffer.data(), got);
    }
    return reply;
}
struct PublicState {
    bool initialized = false, authenticated = false, busy = false, saved = false;
    std::int64_t expiry{};
    double lastVerified{};
    std::string stage = "Preparing your workspace", errorCode, message;
};
class Client {
    GameDefinition game_;
    LicenseCache cache_;
    std::mutex mutex_;
    std::thread worker_;
    std::atomic<bool> cancel_{false}, busy_{false};
    PublicState state_;
    std::string session_;
    double lastCheck_{};
    static double clock() {
        return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }
    std::string fields(const char *type) {
        return std::string("type=") + type + "&name=" + formEncode(game_.appName) +
               "&ownerid=" + formEncode(game_.ownerId);
    }
    Json request(std::string data, bool check = false) {
        WipeAtEnd clean{data};
        auto reply = post(data, check, cancel_);
        WipeAtEnd bodyClean{reply.body};
        if (reply.body == "KeyAuth_Invalid")
            throw Error("application_not_found", "The KeyAuth application was not found. Contact support.");
        verifyResponse(reply.body, reply.signature, reply.timestamp,
                       static_cast<std::int64_t>(std::time(nullptr)));
        auto obj = verifiedJson(reply.body, game_.ownerId);
        if (!obj["success"].get<bool>())
            throw Error("service_denied", serviceError(obj));
        return obj;
    }
    void stage(const std::string &text) {
        std::lock_guard<std::mutex> lock(mutex_);
        state_.stage = text;
    }
    void initializeSync() {
        stage("Connecting to KeyAuth");
        auto reply =
            request(fields("init") + "&ver=" + formEncode(game_.authVersion) + "&hash=" + executableMd5());
        if (!reply.contains("sessionid") || !reply["sessionid"].is_string())
            throw Error("session_missing", "KeyAuth did not create a usable session.");
        std::string id = reply["sessionid"].get<std::string>();
        if (id.empty() || id.size() > 256)
            throw Error("session_format", "The authentication session is not valid.");
        session_ = std::move(id);
        std::lock_guard<std::mutex> lock(mutex_);
        state_.initialized = true;
        state_.stage = "Ready to sign in";
    }
    void loginSync(std::string key, std::string code, bool remember) {
        WipeAtEnd cleanKey{key}, cleanCode{code};
        stage("Validating your license");
        auto reply =
            request(fields("license") + "&sessionid=" + formEncode(session_) + "&key=" + formEncode(key) +
                    "&code=" + formEncode(code) + "&hwid=" + windowsIdentity(game_));
        const auto expiry = subscriptionExpiry(reply, static_cast<std::int64_t>(std::time(nullptr)));
        std::string storageMessage;
        if (remember) {
            try {
                cache_.save(game_, key);
            } catch (const Error &error) {
                storageMessage = error.what();
            }
        } else
            cache_.remove(game_);
        std::lock_guard<std::mutex> lock(mutex_);
        state_.authenticated = true;
        state_.expiry = expiry;
        state_.lastVerified = clock();
        state_.saved = cache_.exists(game_);
        state_.stage = "License verified";
        state_.errorCode.clear();
        state_.message = storageMessage;
    }
    template <class Function> void begin(Function operation) {
        if (busy_)
            return;
        if (worker_.joinable())
            worker_.join();
        busy_ = true;
        cancel_ = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            state_.busy = true;
            state_.errorCode.clear();
            state_.message.clear();
        }
        worker_ = std::thread([this, operation = std::move(operation)]() mutable {
            try {
                operation();
            } catch (const Error &error) {
                wipe(session_);
                std::lock_guard<std::mutex> lock(mutex_);
                state_.initialized = false;
                state_.authenticated = false;
                state_.errorCode = error.code;
                state_.message = error.what();
                state_.stage = "Action required";
            } catch (...) {
                wipe(session_);
                std::lock_guard<std::mutex> lock(mutex_);
                state_.initialized = false;
                state_.authenticated = false;
                state_.errorCode = "unexpected_response";
                state_.message = "The authentication operation could not be completed. Please retry.";
                state_.stage = "Action required";
            }
            {
                std::lock_guard<std::mutex> lock(mutex_);
                state_.busy = false;
            }
            busy_ = false;
        });
    }

  public:
    explicit Client(const GameDefinition &game, std::filesystem::path cacheRoot = {})
        : game_(game), cache_(std::move(cacheRoot)) {}
    ~Client() {
        cancel_ = true;
        if (worker_.joinable())
            worker_.join();
        wipe(session_);
    }
    void wait() {
        if (worker_.joinable())
            worker_.join();
    }
    PublicState snapshot() {
        std::lock_guard<std::mutex> lock(mutex_);
        auto copy = state_;
        copy.busy = busy_.load();
        return copy;
    }
    bool authorized() {
        auto s = snapshot();
        return s.authenticated && s.expiry > std::time(nullptr) && clock() - s.lastVerified < 90;
    }
    void initialize(bool useSaved) {
        if (!game_.available || std::string(game_.ownerId).empty())
            return;
        begin([this, useSaved] {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                state_.authenticated = false;
                state_.initialized = false;
                state_.expiry = 0;
            }
            wipe(session_);
            initializeSync();
            if (useSaved) {
                auto key = cache_.load(game_);
                if (key) {
                    loginSync(std::move(*key), "", true);
                }
            }
        });
    }
    void login(std::string key, std::string code, bool remember) {
        if (key.empty() || key.size() > 255)
            return;
        begin([this, key = std::move(key), code = std::move(code), remember]() mutable {
            if (session_.empty())
                initializeSync();
            loginSync(std::move(key), std::move(code), remember);
        });
    }
    void tick() {
        auto s = snapshot();
        double now = clock();
        if (s.authenticated && !busy_ && now - lastCheck_ >= 60 && now - s.lastVerified >= 60) {
            lastCheck_ = now;
            begin([this] {
                stage("Checking your session");
                request(fields("check") + "&sessionid=" + formEncode(session_), true);
                std::lock_guard<std::mutex> lock(mutex_);
                state_.lastVerified = clock();
                state_.stage = "License verified";
            });
        }
    }
    void forget() {
        if (busy_)
            return;
        if (worker_.joinable())
            worker_.join();
        cache_.remove(game_);
        wipe(session_);
        std::lock_guard<std::mutex> lock(mutex_);
        state_ = {};
        state_.stage = "Signed out";
    }
};
} // namespace v6x::licensing
