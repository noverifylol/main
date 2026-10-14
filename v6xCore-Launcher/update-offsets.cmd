@echo off
setlocal
cd /d "%~dp0"
echo Close any running v6xCore instance first.
start "" /wait "%~dp0v6xCore Launcher.exe" --update-offsets
set "RESULT=%errorlevel%"
echo Result code: %RESULT%
echo The report is saved as diagnostics.txt in the application data folder.
pause
exit /b %RESULT%
