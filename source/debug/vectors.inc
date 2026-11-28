; Фиксированная таблица совместимости F800h
; Порядок включения задан главным .asm; см. README.md.

;----------------------------------------------------------------------------
; Таблица переходов F800h
;----------------------------------------------------------------------------

    ; Это локальные входы DEBUG, не стандартный ABI ядра.
    ORG_PAD0 00700h


debug_restart:       JP    RestartNoCls          ; F800
debug_getch:         JP    mon2_getchOld         ; F803
                    JP    mon2_tapeReadOld      ; F806
debug_printChar:     JP    mon2_printCharOld     ; F809
                    JP    mon2_tapeWriteOld     ; F80C
                    JP    debugPrinterNative          ; F80F
                    JP    setMIfKeyPressed      ; F812
debug_printHexByte:  JP    mx_printHexByte     ; F815
debug_printString:   JP    mon2_printStringOld   ; F818
debug_keyScan:       JP    debugKeyScanNative       ; F81B
                    JP    mx_getCursorPos     ; F81E
                    RET                         ; F821 - не используется
                    NOP
                    NOP
debug_tapeLoad:      JP    debugTapeLoadNative         ; F824
debug_tapeSave:      JP    debugTapeSaveNative         ; F827
debug_calcCS:        JP    bios_calcCS           ; F82A
                    JP    mon2_beep_Old         ; F82D
                    JP    mx_getMemTop        ; F830
                    JP    mx_setMemTop        ; F833
debug_printHexWord:  PUSH  BC                    ; F836 - печать слова из HL в HEX формате, дополняется пробелом
                    JP    printHexWordHL_noSpace

;----------------------------------------------------------------------------
