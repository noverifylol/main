#pragma once
#include "memory.hpp"
#include <shlobj.h>
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace v6x {
inline std::string utf8(const std::wstring &text) {
    if (text.empty())
        return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr,
                                nullptr);
    std::string out(static_cast<size_t>(std::max(0, n)), 0);
    if (n)
        WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), out.data(), n, nullptr,
                            nullptr);
    return out;
}
inline std::wstring wide(const std::string &text) {
    if (text.empty())
        return {};
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()),
                                nullptr, 0);
    if (n <= 0)
        throw std::runtime_error("Invalid UTF-8 text");
    std::wstring out(static_cast<size_t>(n), 0);
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), out.data(),
                        n);
    return out;
}
inline std::filesystem::path exeDirectory() {
    std::wstring name(32768, 0);
    DWORD n = GetModuleFileNameW(nullptr, name.data(), static_cast<DWORD>(name.size()));
    if (!n || n >= name.size())
        throw std::runtime_error("Executable path could not be resolved");
    name.resize(n);
    return std::filesystem::path(name).parent_path();
}
inline std::string readBytes(const std::filesystem::path &p, size_t limit = 1024 * 1024) {
    std::ifstream in(p, std::ios::binary);
    if (!in)
        throw std::runtime_error("File missing or unreadable: " + utf8(p.wstring()));
    in.seekg(0, std::ios::end);
    auto end = in.tellg();
    if (end < 0 || static_cast<std::uint64_t>(end) > limit)
        throw std::runtime_error("File too large or invalid: " + utf8(p.wstring()));
    in.seekg(0);
    std::string text(static_cast<size_t>(end), 0);
    if (!text.empty() && !in.read(text.data(), static_cast<std::streamsize>(text.size())))
        throw std::runtime_error("Read failed: " + utf8(p.wstring()));
    return text;
}
inline std::string utcNow() {
    SYSTEMTIME t{};
    GetSystemTime(&t);
    char b[40]{};
    std::snprintf(b, sizeof(b), "%04u-%02u-%02uT%02u:%02u:%02uZ", t.wYear, t.wMonth, t.wDay, t.wHour,
                  t.wMinute, t.wSecond);
    return b;
}
inline void atomicWrite(const std::filesystem::path &path, const std::string &bytes, bool backup = false) {
    std::filesystem::create_directories(path.parent_path());
    auto temp = path;
    temp += L".new-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64());
    try {
        {
            std::ofstream out(temp, std::ios::binary | std::ios::trunc);
            if (!out)
                throw std::runtime_error("Cannot create: " + utf8(temp.wstring()));
            out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
            out.flush();
            if (!out)
                throw std::runtime_error("Could not write complete file");
        }
        if (backup && std::filesystem::exists(path)) {
            auto bak = path;
            bak += L".backup-" + std::to_wstring(GetTickCount64());
            std::filesystem::copy_file(path, bak, std::filesystem::copy_options::none);
        }
        if (!MoveFileExW(temp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            throw std::runtime_error("File replacement failed, Windows error " +
                                     std::to_string(GetLastError()));
    } catch (...) {
        std::error_code e;
        std::filesystem::remove(temp, e);
        throw;
    }
}
inline bool writableDirectory(const std::filesystem::path &dir) {
    std::error_code e;
    std::filesystem::create_directories(dir, e);
    if (e)
        return false;
    auto probe = dir / (L".v6x-write-probe-" + std::to_wstring(GetCurrentProcessId()) + L"-" +
                        std::to_wstring(GetTickCount64()));
    HANDLE file =
        CreateFileW(probe.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return false;
    CloseHandle(file);
    DeleteFileW(probe.c_str());
    return true;
}
inline std::filesystem::path dataDirectory(const std::filesystem::path &executable,
                                           const std::filesystem::path &overridePath = {}) {
    if (!overridePath.empty()) {
        auto p = std::filesystem::absolute(overridePath);
        if (!writableDirectory(p))
            throw std::runtime_error("Data directory is not writable: " + utf8(p.wstring()));
        return p;
    }
    if (writableDirectory(executable))
        return executable;
    PWSTR path{};
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, nullptr, &path);
    if (FAILED(hr) || !path)
        throw std::runtime_error("No writable data directory found");
    std::filesystem::path fallback = std::filesystem::path(path) / L"v6xCore" / L"Cs2External";
    CoTaskMemFree(path);
    if (!writableDirectory(fallback))
        throw std::runtime_error("LocalAppData fallback is not writable");
    return fallback;
}
inline std::string bundledText(int id) {
    HRSRC entry = FindResourceW(nullptr, MAKEINTRESOURCEW(id), RT_RCDATA);
    if (!entry)
        throw std::runtime_error("Embedded resource missing: " + std::to_string(id));
    HGLOBAL handle = LoadResource(nullptr, entry);
    void *p = handle ? LockResource(handle) : nullptr;
    DWORD n = SizeofResource(nullptr, entry);
    if (!p || !n)
        throw std::runtime_error("Embedded resource is empty");
    return {static_cast<const char *>(p), n};
}
inline void ensureUtf8Bom(const std::filesystem::path &path) {
    std::string text;
    if (std::filesystem::exists(path))
        text = readBytes(path, 2 * 1024 * 1024);
    if (text.compare(0, 3, "\xef\xbb\xbf") != 0)
        atomicWrite(path, std::string("\xef\xbb\xbf") + text);
}
} // namespace v6x
