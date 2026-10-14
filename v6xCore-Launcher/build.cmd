@echo off
setlocal
cd /d "%~dp0"
where cl >nul 2>nul
if errorlevel 1 (
 echo Open the x64 Native Tools Command Prompt for Visual Studio 2022 first.
 exit /b 1
)
if not "%VSCMD_ARG_TGT_ARCH%"=="x64" (
 echo An x64 compiler environment is required.
 exit /b 1
)
if not exist artifacts mkdir artifacts
rc /nologo /I . /I assets /fo artifacts\app.res assets\app.rc
if errorlevel 1 exit /b 1
cl /nologo /TC /O2 /MT /c vendor\monocypher\monocypher.c /Foartifacts\monocypher.obj
if errorlevel 1 exit /b 1
cl /nologo /TC /O2 /MT /c vendor\monocypher\monocypher-ed25519.c /Foartifacts\monocypher-ed25519.obj
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /O2 /EHsc /MT /W4 /utf-8 /guard:cf /DUNICODE /D_UNICODE /DNOMINMAX /D_WIN32_WINNT=0x0A00 /I vendor\imgui /I vendor\imgui\backends src\main.cpp vendor\imgui\imgui.cpp vendor\imgui\imgui_draw.cpp vendor\imgui\imgui_tables.cpp vendor\imgui\imgui_widgets.cpp vendor\imgui\backends\imgui_impl_dx11.cpp vendor\imgui\backends\imgui_impl_win32.cpp artifacts\monocypher.obj artifacts\monocypher-ed25519.obj artifacts\app.res /Foartifacts\ /Fe:"v6xCore Launcher.exe" /link /SUBSYSTEM:WINDOWS /DYNAMICBASE /NXCOMPAT /HIGHENTROPYVA /guard:cf d3d11.lib d3dcompiler.lib dxgi.lib dwmapi.lib gdiplus.lib ole32.lib uuid.lib shell32.lib user32.lib gdi32.lib imm32.lib winhttp.lib bcrypt.lib crypt32.lib advapi32.lib
if errorlevel 1 exit /b 1
for %%T in (math profile_name data team_rules license_protocol) do (
 cl /nologo /std:c++17 /O2 /EHsc /MT /utf-8 tests\test_%%T.cpp artifacts\monocypher.obj artifacts\monocypher-ed25519.obj /Foartifacts\test_%%T.obj /Fe:artifacts\test_%%T.exe
 if errorlevel 1 exit /b 1
 artifacts\test_%%T.exe
 if errorlevel 1 exit /b 1
)
echo Native build and portable C++ tests passed.
echo See CMakeLists.txt for the additional Win32 test targets.
