@echo off
if "%~4"=="" (
  powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0build-if-needed.ps1" "%~1" "%~2" "%~3"
) else (
  powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0build-if-needed.ps1" "%~1" "%~2" "%~3" "%~4"
)
exit /b %errorlevel%
