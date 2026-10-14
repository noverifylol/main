@echo off
cd /d "%~dp0"
echo UI preview only. Authentication, game-process access and mouse output are disabled.
start "" "%~dp0v6xCore Launcher.exe" --ui-test
