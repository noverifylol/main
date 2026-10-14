#include "../src/config_name.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>
int main() {
    using cs2::safeConfigName;
    assert(safeConfigName("Balanced").value() == "Balanced");
    assert(safeConfigName("  My profile-02  ").value() == "My profile-02");
    for (const char *bad : {"", " ", "..", "../secret", "C:\\test", "/tmp", "bad.name", "CON", "nul", "LPT1",
                            "COM4", "a:b", "a?b", "a*b"})
        assert(!safeConfigName(bad));
    assert(!safeConfigName(std::string(41, 'x')));
    assert(safeConfigName(std::string(40, 'x')));
    std::cout << "PASS: profile names, trimming, traversal rejection and Windows reserved names\n";
}
