;+---------------------------------------------------------------------------
; RAMFOS
; Точки входа
;
; 2013-11-01 Дизассемблировано vinxru
;----------------------------------------------------------------------------

			jp init
			jp j_keyWait
			jp j_tapeRead
			jp j_printChar
			jp j_tapeWrite
			jp j_printCharA
			jp j_keyCheck
			jp j_printHex
			jp j_printString
			jp j_keyScan
			jp j_getCursorPos
			jp j_getCharFromScr
			jp ramfosLoadDialog
			jp ramfosSaveDialog
			jp j_calcCrc
			jp fileSetType
			jp j_getMemTop
			jp j_setMemTop
j_pageReadAny:	jp ramfosReadAny
j_pageWriteAny:	jp ramfosWriteAny
			jp j_setCursorPos
			jp ramfosSave
			jp ramfosSetAddr
			jp ramfosGetName
			jp ramfosSetName
			jp ramfosFind
			jp ramfosLoad
			jp ramfosDelete
    IF NO_ROM_ENTRY | RKS_LOADER
    IF NO_MXDOS
			jp init
    ELSE
			jp loadMxDos
    ENDIF
			jp ramfosExec
    ENDIF

    IF MXOS_KERNEL
ramfosStart:
    jp compatStart
    ASSERT ramfosStart == 0F85Ah
    ENDIF
