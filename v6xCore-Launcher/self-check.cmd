@echo off
setlocal
cd /d "%~dp0"
echo Close any running v6xCore instance first.
start "" /wait "%~dp0v6xCore Launcher.exe" --self-check
set "RESULT=%errorlevel%"
echo Result code: %RESULT%
echo This checks local startup data. It does not access CS2 or validate a customer license.
pause
exit /b %RESULT%
