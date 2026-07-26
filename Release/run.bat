@echo off
call "%~dp0fix_listings_for_emu.bat"
if errorlevel 1 exit /b %errorlevel%

pushd "%~dp0..\ROM\makeRom"
cscript.exe //nologo ".\-makeRom.js"
set "runError=%errorlevel%"
popd
if not "%runError%"=="0" exit /b %runError%

pushd "%~dp0..\FlashDrive\Games64k"
cscript.exe //nologo ".\-makeRom.js"
set "runError=%errorlevel%"
popd
if not "%runError%"=="0" exit /b %runError%

powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0run.ps1"
exit /b %errorlevel%
