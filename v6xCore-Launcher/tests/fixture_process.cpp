#include <windows.h>
// A deliberately inert process-name fixture, not CS2 and not a game-memory mock.
// Build only for the diagnostic test as artifacts/fixture/cs2.exe. No client.dll is loaded.
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    Sleep(45000);
    return 0;
}
