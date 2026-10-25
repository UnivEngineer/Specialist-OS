; Фиксированная таблица совместимости F800h
; Порядок включения задан главным .asm; см. README.md.

;----------------------------------------------------------------------------
; Таблица переходов F800h
;----------------------------------------------------------------------------

    ORG_PAD0 0F800h


mon2_restart:       JP    RestartNoCls          ; F800
mon2_getch:         JP    bios_getchOld         ; F803
                    JP    bios_tapeReadOld      ; F806
mon2_printChar:     JP    bios_printCharOld     ; F809
                    JP    bios_tapeWriteOld     ; F80C
                    JP    bios_printer          ; F80F
                    JP    setMIfKeyPressed      ; F812
mon2_printHexByte:  JP    bios_printHexByte     ; F815
mon2_printString:   JP    bios_printStringOld   ; F818
mon2_keyScan:       JP    bios_keyScanOld       ; F81B
                    JP    bios_getCursorPos     ; F81E
                    RET                         ; F821 - не используется
                    NOP
                    NOP
mon2_tapeLoad:      JP    bios_tapeLoad         ; F824
mon2_tapeSave:      JP    bios_tapeSave         ; F827
mon2_calcCS:        JP    bios_calcCS           ; F82A
                    JP    bios_beep_Old         ; F82D
                    JP    bios_getMemTop        ; F830
                    JP    bios_setMemTop        ; F833
mon2_printHexWord:  PUSH  BC                    ; F836 - печать слова из HL в HEX формате, дополняется пробелом
                    JP    printHexWordHL_noSpace

;----------------------------------------------------------------------------
