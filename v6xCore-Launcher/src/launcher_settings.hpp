#pragma once
#include "settings.hpp"
#include "file_io.hpp"
namespace v6x {
struct LauncherSettings {
    bool remember = true, animate = true, capturePrivacy = false;
    int uiHz = 120;
    std::filesystem::path file;
    explicit LauncherSettings(const std::filesystem::path &directory) : file(directory / "launcher.ini") {
        remember = GetPrivateProfileIntW(L"launcher", L"remember_license", 1, file.c_str()) != 0;
        animate = GetPrivateProfileIntW(L"launcher", L"animation", 1, file.c_str()) != 0;
        capturePrivacy = GetPrivateProfileIntW(L"launcher", L"capture_privacy", 0, file.c_str()) != 0;
        uiHz = std::clamp(static_cast<int>(GetPrivateProfileIntW(L"launcher", L"ui_hz", 120, file.c_str())),
                          30, 144);
    }
    bool save() const {
        bool ok = true;
        auto put = [&](const wchar_t *k, const wchar_t *v) {
            if (!WritePrivateProfileStringW(L"launcher", k, v, file.c_str()))
                ok = false;
        };
        put(L"remember_license", remember ? L"1" : L"0");
        put(L"animation", animate ? L"1" : L"0");
        put(L"capture_privacy", capturePrivacy ? L"1" : L"0");
        auto rate = std::to_wstring(uiHz);
        put(L"ui_hz", rate.c_str());
        return ok;
    }
};
} // namespace v6x
