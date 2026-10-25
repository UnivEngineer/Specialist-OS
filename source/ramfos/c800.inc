;+---------------------------------------------------------------------------
; RAMFOS
; Точки входа
;
; 2013-11-01 Дизассемблировано vinxru
;----------------------------------------------------------------------------

j_biosInit:	jp biosInit
j_keyWait:	jp keyWait
j_tapeRead:	jp tapeRead
j_printChar:	jp printChar
j_tapeWrite:	jp tapeWrite
j_printCharA:	jp printCharA
j_keyCheck:	jp keyCheck
j_printHex:	jp printHex
j_printString:	jp printString
j_keyScan:	jp keyScan
j_getCursorPos:	jp getCursorPos
j_getCharFromScr:	jp getCharFromScreen
j_tapeReadPilot:	jp tapeReadPilot
j_tapeWritePilot:	jp tapeWritePilot
j_calcCrc:	jp calcCrc
j_setBottomLine:	jp setBottomLine
j_getMemTop:	jp getMemTop
j_setMemTop:	jp setMemTop
j_input:	jp input
j_strToHex:	jp strToHex
j_setCursorPos:	jp setCursorPos
			jp printer
			jp printerHex
j_inputFileName:	jp inputFileName
			jp beep
			jp setColor
			jp fullClearScreen
			jp oldBeep
j_cmp_hl_de:	jp cmp_hl_de
j_printerEchoMode:	jp printerEchoMode
j_tapeMode:	jp tapeMode
