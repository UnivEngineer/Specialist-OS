call fix_listings_for_emu.bat
cd ..\ROM\makeRom
-makeRom.js
cd ..\..\..\..\Emulator\emu
EMU.exe /c "SpecialistMX2_My_MXOS"
