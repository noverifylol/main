#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p artifacts
python3 tools/update_offsets.py --offline
for test in math profile_name data team_rules; do
  g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror "tests/test_${test}.cpp" -o "artifacts/test_${test}"
  "./artifacts/test_${test}"
done
gcc -std=c99 -O2 -c vendor/monocypher/monocypher.c -o artifacts/native_monocypher.o
gcc -std=c99 -O2 -c vendor/monocypher/monocypher-ed25519.c -o artifacts/native_ed25519.o
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror tests/test_license_protocol.cpp artifacts/native_monocypher.o artifacts/native_ed25519.o -o artifacts/test_license_protocol
./artifacts/test_license_protocol
python3 -m unittest discover -s tests -p 'test_*.py' -v
CXX=x86_64-w64-mingw32-g++
common=(-std=c++17 -O2 -DUNICODE -D_UNICODE -DNOMINMAX -D_WIN32_WINNT=0x0A00 -Ivendor/imgui -Ivendor/imgui/backends)
objects=()
for file in vendor/imgui/imgui.cpp vendor/imgui/imgui_draw.cpp vendor/imgui/imgui_tables.cpp vendor/imgui/imgui_widgets.cpp vendor/imgui/backends/imgui_impl_dx11.cpp vendor/imgui/backends/imgui_impl_win32.cpp; do
  obj="artifacts/$(basename "${file%.cpp}").o"
  if [[ ! -f "$obj" || "$file" -nt "$obj" ]]; then "$CXX" "${common[@]}" -w -c "$file" -o "$obj"; fi
  objects+=("$obj")
done
for file in vendor/monocypher/monocypher.c vendor/monocypher/monocypher-ed25519.c; do
  obj="artifacts/$(basename "${file%.c}").o"
  x86_64-w64-mingw32-gcc -std=c99 -O2 -c "$file" -o "$obj"
  objects+=("$obj")
done
x86_64-w64-mingw32-windres -I . -I assets assets/app.rc -O coff -o artifacts/app.res
"$CXX" "${common[@]}" -Wall -Wextra -Wpedantic -Werror -c src/main.cpp -o artifacts/main.o
"$CXX" artifacts/main.o "${objects[@]}" artifacts/app.res -o 'v6xCore Launcher.exe' \
  -Wl,--dynamicbase,--nxcompat,--high-entropy-va -s -municode -mwindows -static -static-libgcc -static-libstdc++ \
  -ld3d11 -ld3dcompiler -ldxgi -ldwmapi -lgdiplus -lole32 -luuid -lshell32 -luser32 -lgdi32 -limm32 -lwinhttp -lbcrypt -lwinpthread -lcrypt32 -ladvapi32
"$CXX" -std=c++17 -O2 -DUNICODE -D_UNICODE -DNOMINMAX -D_WIN32_WINNT=0x0A00 tests/test_settings.cpp \
  -o artifacts/test_settings.exe -static -static-libgcc -static-libstdc++ -luser32
"$CXX" -std=c++17 -O2 -DUNICODE -D_UNICODE -DNOMINMAX -D_WIN32_WINNT=0x0A00 tests/test_bootstrap.cpp artifacts/app.res \
  -o artifacts/test_bootstrap.exe -static -static-libgcc -static-libstdc++ -lshell32 -lole32 -luuid -luser32
for test in license_cache target_selection; do
  "$CXX" -std=c++17 -O2 -DUNICODE -D_UNICODE -DNOMINMAX -D_WIN32_WINNT=0x0A00 "tests/test_${test}.cpp" artifacts/monocypher.o artifacts/monocypher-ed25519.o \
    -o "artifacts/test_${test}.exe" -static -static-libgcc -static-libstdc++ -lcrypt32 -ladvapi32 -lwinhttp -lbcrypt -lwinpthread -lshell32 -lole32 -luuid -luser32
done
printf '\nWINDOWS_X64_BUILD_OK\n'  
