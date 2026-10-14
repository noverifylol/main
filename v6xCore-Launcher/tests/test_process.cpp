#include "../src/memory.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>
int main() {
    cs2::Process p;
    bool attached = p.attach();
    std::cout << "Detected PID: " << p.foundPid() << "; attached PID: " << p.pid() << "\n";
    assert(p.foundPid() != 0);
    assert(!attached && p.pid() == 0);
    assert(p.status.find(L"client.dll") != std::wstring::npos);
    std::cout
        << "PASS: cs2.exe-name fixture detected; missing client.dll is distinguished from missing process\n";
}
