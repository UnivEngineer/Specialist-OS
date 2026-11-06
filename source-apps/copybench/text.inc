; Строки COPYBEN. Отступы перед временем задаются пробелами.
; RAM-строка: имя + pairText + запись + pairSeparator + чтение.
; ROM-строка: directText/blockText/byteText + число той же ширины.
title:        DB "COPYBEN: 8192 bytes",10,"PIT 2 MHz; physical RAM page ",0
working:      DB 10,"Saving RAM contents...",10,0
keyText:      DB "Press any key to exit.",0
okText:       DB " ms  OK",10,0
failText:     DB " ms  VERIFY FAILED",10,0
exitNewline:  DB 10,0
doneText:     DB "RAM and driver restored. ",0
timerMessage: DB "PIT clock stopped / invalid",10,0
noRAMText:    DB "No RAM expansion detected.",10,0
noMemoryText: DB "COPYBEN needs memory through 88FFh.",10,0
directText:   DB "Regs R4    ROM   -> RAMD ",0
blockText:    DB "BIOS block ROM   -> RAMD ",0
byteText:     DB "BIOS byte  ROM   -> RAMD ",0
blockName:    DB "BIOS block",0
byteName:     DB "BIOS byte ",0
cpmName:      DB "CP/M buf16",0
stackName:    DB "Stack     ",0
loopName:     DB "Loop      ",0
unrolledName: DB "Unrolled  ",0
directName:   DB "Direct    ",0
regs2Name:    DB "Regs R2   ",0
regs4Name:    DB "Regs R4   ",0
regs6Name:    DB "Regs R6   ",0
pairText:     DB " RAM  <-> RAMD ",0
pairSeparator: DB " ms / ",0
pairFailedSeparator: DB " ms ! / ",0

farText:      DB "FAR R4     RAMD --> RAMD ",0
farSkipText:  DB "RAMD --> RAMD: R4 and CP/M need 2 pages",10,0
cpmFarText:   DB "FAR CP/M   RAMD --> RAMD ",0

r4BlockName:      DB "R4  blk256",0
cpmBlockName:     DB "CPM blk256",0
r4UniversalName:  DB "R4  univ  ",0
cpmUniversalName: DB "CPM univ  ",0
engineBankText:   DB " RAMD --> RAMD ",0
