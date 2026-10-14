#include "../src/updater.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>
int main() {
    using namespace v6x;
    assert(sha256("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    wchar_t temp[MAX_PATH]{};
    GetTempPathW(MAX_PATH, temp);
    auto directory =
        std::filesystem::path(temp) / (L"v6x-updater-test-" + std::to_wstring(GetCurrentProcessId()));
    std::filesystem::create_directories(directory);
    atomicWrite(directory / L"offsets.ini", "unchanged");
    std::atomic<bool> cancel{true};
    auto result = updateOnline(directory, {}, &cancel);
    assert(!result.success);
    assert(readBytes(directory / L"offsets.ini") == "unchanged");
    std::filesystem::remove_all(directory);
    std::cout << "PASS: native SHA256 vector; cancelled update preserves the existing file\n";
}
