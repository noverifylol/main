#include "../src/config.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <filesystem>
#include <iostream>
int main() {
    using namespace cs2;
    wchar_t temp[MAX_PATH]{};
    assert(GetTempPathW(MAX_PATH, temp) > 0);
    auto root =
        std::filesystem::path(temp) / (L"v6x-settings-test-" + std::to_wstring(GetCurrentProcessId()));
    std::filesystem::create_directories(root);
    auto path = (root / L"test.ini").wstring();
    Config a;
    a.path = path;
    a.aim = true;
    a.esp = false;
    a.boxStyle = 2;
    a.fov = 8.5f;
    a.snaplines = true;
    a.enemyColor = {.1f, .2f, .3f};
    a.aimKey = VK_XBUTTON2;
    assert(a.save());
    Config b;
    b.load(path);
    assert(!b.aim && !b.esp && b.boxStyle == 2 && b.snaplines && b.aimKey == VK_XBUTTON2);
    assert(std::abs(b.fov - 8.5f) < .001f);
    assert(std::abs(b.enemyColor[2] - .3f) < .001f);
    WritePrivateProfileStringW(L"settings", L"fov_degrees", L"nan", path.c_str());
    b.load(path);
    assert(b.fov == 4);
    WritePrivateProfileStringW(L"settings", L"fov_degrees", L"999", path.c_str());
    b.load(path);
    assert(b.fov == 15);
    WritePrivateProfileStringW(L"settings", L"enemy_0", L"-7", path.c_str());
    b.load(path);
    assert(b.enemyColor[0] == 0);
    WritePrivateProfileStringW(L"settings", L"hold_key", L"INVALID", path.c_str());
    b.load(path);
    assert(b.aimKey == VK_MENU);
    Offsets o;
    assert(!o.load((root / L"missing-offsets.ini").wstring()));
    assert(!o.error.empty());
    std::filesystem::remove_all(root);
    std::cout << "PASS: Win32 INI round-trip, unarmed reload, clamps, invalid values and missing offsets\n";
}
