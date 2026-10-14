#include "../src/bootstrap.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>
int main() {
    using namespace v6x;
    wchar_t temp[MAX_PATH]{};
    assert(GetTempPathW(MAX_PATH, temp) > 0);
    auto dir = std::filesystem::path(temp) / (L"v6x-bootstrap-" + std::to_wstring(GetCurrentProcessId())) /
               L"Prüfung mit Leerzeichen 日本語";
    assert(!std::filesystem::exists(dir));
    std::filesystem::create_directories(dir);
    auto boot = bootstrap(dir);
    assert(boot.offsetsCreated);
    for (auto name : {L"offsets.ini", L"settings.ini", L"configs/Balanced.ini", L"configs/Minimal.ini",
                      L"configs/Ice.ini"})
        assert(std::filesystem::is_regular_file(dir / name));
    cs2::Offsets data;
    assert(data.load((dir / L"offsets.ini").wstring()));
    assert(data.build == 14190);
    assert(data.provider == L"sezzyaep/CS2-OFFSETS");
    std::string original = readBytes(dir / L"offsets.ini");
    atomicWrite(dir / L"offsets.ini", "[globals]\ndwEntityList=INVALID\n");
    assert(!bootstrap(dir).offsetsCreated);
    assert(!data.load((dir / L"offsets.ini").wstring()));
    assert(data.error.find(L"dwEntityList") != std::wstring::npos);
    restoreBundledOffsets(dir);
    assert(data.load((dir / L"offsets.ini").wstring()));
    assert(readBytes(dir / L"offsets.ini") == original);
    bool backup = false;
    for (const auto &entry : std::filesystem::directory_iterator(dir))
        if (entry.path().filename().wstring().find(L"offsets.ini.backup-") == 0) {
            backup = true;
            assert(readBytes(entry.path()).find("INVALID") != std::string::npos);
        }
    assert(backup);
    atomicWrite(dir / L"offsets.ini", std::string("\xef\xbb\xbf") + original);
    assert(data.load((dir / L"offsets.ini").wstring()));
    ensureUtf8Bom(dir / L"test.log");
    assert(readBytes(dir / L"test.log") == "\xef\xbb\xbf");
    assert(dataDirectory(dir) == dir);
    std::filesystem::remove_all(dir.parent_path());
    std::cout << "PASS: embedded bootstrap, moved-EXE data recovery, Unicode paths, corrupt file preserved, "
                 "repair backups and UTF-8 BOM\n";
}
