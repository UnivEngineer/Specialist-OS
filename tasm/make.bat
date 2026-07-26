@echo off
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0build-if-needed.ps1" "%~1" "%~2" "%~3"
exit /b %errorlevel%