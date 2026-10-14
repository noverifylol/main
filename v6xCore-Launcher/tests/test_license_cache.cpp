#include "../src/license_client.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>
int main() {
    using namespace v6x;
    using namespace v6x::licensing;
    wchar_t temp[MAX_PATH]{};
    GetTempPathW(MAX_PATH, temp);
    auto folder =
        std::filesystem::path(temp) / (L"v6x-license-cache-test-" + std::to_wstring(GetCurrentProcessId()));
    assert(!std::filesystem::exists(folder));
    LicenseCache cache(folder);
    const std::string synthetic = "LOCAL-ONLY-UNIT-TEST-NOT-A-REAL-LICENSE";
    cache.save(catalog[0], synthetic);
    assert(cache.load(catalog[0]).value() == synthetic);
    auto encrypted = readBytes(cache.path(catalog[0]));
    assert(encrypted.find(synthetic) == std::string::npos);
    auto other = catalog[0];
    other.id = "isolated-test-game";
    atomicWrite(cache.path(other), encrypted);
    bool rejected = false;
    try {
        cache.load(other);
    } catch (const Error &) {
        rejected = true;
    }
    assert(rejected);
    auto corrupt = encrypted;
    corrupt.back() ^= 0x22;
    atomicWrite(cache.path(catalog[0]), corrupt);
    rejected = false;
    try {
        cache.load(catalog[0]);
    } catch (const Error &) {
        rejected = true;
    }
    assert(rejected);
    cache.remove(catalog[0]);
    assert(!cache.exists(catalog[0]));
    assert(windowsIdentity(catalog[0]).size() == 64);
    assert(windowsIdentity(catalog[0]) != windowsIdentity(other));
    std::filesystem::remove_all(folder);
    std::cout << "PASS: Windows DPAPI round-trip, no plaintext at rest, per-game binding, tamper rejection "
                 "and app-scoped identity\n";
}
