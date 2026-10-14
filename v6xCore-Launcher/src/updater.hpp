#pragma once
#include "file_io.hpp"
#include "offset_data.hpp"
#include "config.hpp"
#include <winhttp.h>
#include <bcrypt.h>
#include <atomic>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace v6x {
class HttpHandle {
    HINTERNET h_{};

  public:
    explicit HttpHandle(HINTERNET h) : h_(h) {}
    ~HttpHandle() {
        if (h_)
            WinHttpCloseHandle(h_);
    }
    HttpHandle(const HttpHandle &) = delete;
    HttpHandle &operator=(const HttpHandle &) = delete;
    operator HINTERNET() const {
        return h_;
    }
};
inline std::runtime_error httpError(const char *step) {
    return std::runtime_error(std::string(step) + ": Windows error " + std::to_string(GetLastError()));
}
inline std::string httpGet(const std::wstring &host, const std::wstring &path,
                           std::atomic<bool> *cancel = nullptr) {
    if (cancel && cancel->load())
        throw std::runtime_error("Update cancelled");
    HttpHandle session(WinHttpOpen(L"v6xCore/3.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
                                   WINHTTP_NO_PROXY_BYPASS, 0));
    if (!session)
        throw httpError("WinHttpOpen");
    WinHttpSetTimeouts(session, 5000, 5000, 10000, 10000);
    HttpHandle connection(WinHttpConnect(session, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0));
    if (!connection)
        throw httpError("WinHttpConnect");
    HttpHandle request(WinHttpOpenRequest(connection, L"GET", path.c_str(), nullptr, WINHTTP_NO_REFERER,
                                          WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE));
    if (!request)
        throw httpError("WinHttpOpenRequest");
    DWORD redirects = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    WinHttpSetOption(request, WINHTTP_OPTION_REDIRECT_POLICY, &redirects, sizeof(redirects));
    if (!WinHttpSendRequest(request, L"Accept: application/json\r\n", static_cast<DWORD>(-1),
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0))
        throw httpError("HTTPS request");
    if (!WinHttpReceiveResponse(request, nullptr))
        throw httpError("HTTPS response");
    DWORD status{}, size = sizeof(status);
    if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX))
        throw httpError("HTTP status");
    if (status != 200)
        throw std::runtime_error("HTTP " + std::to_string(status) + " from " + utf8(host));
    std::string body;
    std::array<char, 65536> buffer{};
    for (;;) {
        if (cancel && cancel->load())
            throw std::runtime_error("Update cancelled");
        DWORD got{};
        if (!WinHttpReadData(request, buffer.data(), static_cast<DWORD>(buffer.size()), &got))
            throw httpError("HTTP read");
        if (!got)
            break;
        if (body.size() + got > 8 * 1024 * 1024)
            throw std::runtime_error("HTTP response exceeds 8 MiB");
        body.append(buffer.data(), got);
    }
    return body;
}
inline std::string sha256(const std::string &data) {
    BCRYPT_ALG_HANDLE algorithm{};
    BCRYPT_HASH_HANDLE hash{};
    std::vector<UCHAR> object;
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0)
        throw std::runtime_error("SHA256 initialization failed");
    try {
        DWORD objectSize{}, got{};
        if (BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&objectSize),
                              sizeof(objectSize), &got, 0) < 0 ||
            objectSize > 1024 * 1024)
            throw std::runtime_error("SHA256 property error");
        object.resize(objectSize);
        std::array<UCHAR, 32> digest{};
        if (BCryptCreateHash(algorithm, &hash, object.data(), objectSize, nullptr, 0, 0) < 0)
            throw std::runtime_error("SHA256 create failed");
        if (BCryptHashData(hash, reinterpret_cast<PUCHAR>(const_cast<char *>(data.data())),
                           static_cast<ULONG>(data.size()), 0) < 0 ||
            BCryptFinishHash(hash, digest.data(), static_cast<ULONG>(digest.size()), 0) < 0)
            throw std::runtime_error("SHA256 digest failed");
        BCryptDestroyHash(hash);
        hash = nullptr;
        BCryptCloseAlgorithmProvider(algorithm, 0);
        algorithm = nullptr;
        std::ostringstream out;
        out << std::hex << std::setfill('0');
        for (auto byte : digest)
            out << std::setw(2) << static_cast<unsigned>(byte);
        return out.str();
    } catch (...) {
        if (hash)
            BCryptDestroyHash(hash);
        if (algorithm)
            BCryptCloseAlgorithmProvider(algorithm, 0);
        throw;
    }
}
struct UpdateResult {
    bool success{};
    std::string message, commit;
};
inline UpdateResult updateOnline(const std::filesystem::path &directory,
                                 const std::function<void(const std::string &)> &progress = {},
                                 std::atomic<bool> *cancel = nullptr) {
    try {
        auto stage = [&](const std::string &msg) {
            if (progress)
                progress(msg);
        };
        stage("Checking source revision ...");
        auto head =
            Json::parse(httpGet(L"api.github.com", L"/repos/sezzyaep/CS2-OFFSETS/commits/main", cancel));
        std::string commit = head.at("sha").get<std::string>();
        if (!validCommit(commit))
            throw std::runtime_error("Source returned an invalid commit");
        std::wstring prefix = L"/sezzyaep/CS2-OFFSETS/" + wide(commit) + L"/";
        stage("Downloading global offsets ...");
        std::string globals = httpGet(L"raw.githubusercontent.com", prefix + L"offsets.json", cancel);
        stage("Downloading schema ...");
        std::string schema = httpGet(L"raw.githubusercontent.com", prefix + L"client_dll.json", cancel);
        stage("Downloading build metadata ...");
        auto info = Json::parse(httpGet(L"raw.githubusercontent.com", prefix + L"info.json", cancel));
        unsigned build = static_cast<unsigned>(checkedNumber(info.at("build_number"), 10000000));
        SnapshotMeta meta{"https://github.com/sezzyaep/CS2-OFFSETS",
                          "sezzyaep/CS2-OFFSETS",
                          commit,
                          head.at("commit").at("committer").at("date").get<std::string>(),
                          utcNow(),
                          build};
        stage("Validating data ...");
        std::string ini = makeOffsetIni(Json::parse(globals), Json::parse(schema), meta);
        const std::string hashes =
            "raw_offsets_sha256 = " + sha256(globals) + "\nraw_schema_sha256 = " + sha256(schema) + "\n";
        ini.insert(ini.find("\n\n[globals]"), "\n" + hashes);
        cs2::Offsets check;
        if (!check.loadText(ini))
            throw std::runtime_error("Generated data rejected: " + utf8(check.error));
        if (cancel && cancel->load())
            throw std::runtime_error("Update cancelled");
        atomicWrite(directory / "offsets.ini", ini, true);
        return {true, "Offsets updated: build " + std::to_string(build) + ". Aim control switched off.",
                commit};
    } catch (const std::exception &error) {
        return {false, "Update failed; local file was not replaced: " + std::string(error.what()), {}};
    }
}
class OffsetUpdater {
    std::thread thread_;
    std::mutex mutex_;
    std::string stage_;
    UpdateResult result_;
    std::atomic<bool> busy_{false}, done_{false}, cancel_{false};

  public:
    ~OffsetUpdater() {
        cancel_ = true;
        if (thread_.joinable())
            thread_.join();
    }
    bool busy() const {
        return busy_.load();
    }
    std::string stage() {
        std::lock_guard<std::mutex> lock(mutex_);
        return stage_;
    }
    void start(const std::filesystem::path &directory) {
        if (busy())
            return;
        if (thread_.joinable())
            thread_.join();
        busy_ = true;
        done_ = false;
        cancel_ = false;
        thread_ = std::thread([this, directory] {
            auto result = updateOnline(
                directory,
                [this](const std::string &s) {
                    std::lock_guard<std::mutex> lock(mutex_);
                    stage_ = s;
                },
                &cancel_);
            {
                std::lock_guard<std::mutex> lock(mutex_);
                result_ = std::move(result);
            }
            busy_ = false;
            done_ = true;
        });
    }
    bool take(UpdateResult &out) {
        if (!done_.exchange(false))
            return false;
        if (thread_.joinable())
            thread_.join();
        std::lock_guard<std::mutex> lock(mutex_);
        out = result_;
        return true;
    }
};
} // namespace v6x
