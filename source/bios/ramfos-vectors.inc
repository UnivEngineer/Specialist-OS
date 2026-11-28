; Входы RAMFOS C800h–C85Ah. Общий порядок для начальной копии и установщика.
    IFNDEF BIOS_RAMFOS_VECTOR_MACRO
    DEFINE BIOS_RAMFOS_VECTOR_MACRO
    MACRO RAMFOS_VECTOR target
        IFDEF BIOS_RAMFOS_JUMPS
            jp target
        ELSE
            DW target
        ENDIF
    ENDM
    ENDIF

    RAMFOS_VECTOR ramfosInit                ; C800h
    RAMFOS_VECTOR ramfosKeyWait             ; C803h
    RAMFOS_VECTOR ramfosTapeReadUnavailable ; C806h
    RAMFOS_VECTOR ramfosPrint               ; C809h
    RAMFOS_VECTOR ramfosDeviceNoop          ; C80Ch
    RAMFOS_VECTOR ramfosPrintA              ; C80Fh
    RAMFOS_VECTOR ramfosKeyCheck            ; C812h
    RAMFOS_VECTOR ramfosPrintHex            ; C815h
    RAMFOS_VECTOR ramfosString              ; C818h
    RAMFOS_VECTOR ramfosKeyScan             ; C81Bh
    RAMFOS_VECTOR ramfosGetCursor           ; C81Eh
    RAMFOS_VECTOR ramfosGetGlyph            ; C821h
    RAMFOS_VECTOR ramfosTapeReadUnavailable ; C824h
    RAMFOS_VECTOR ramfosDeviceNoop          ; C827h
    RAMFOS_VECTOR calcCS                    ; C82Ah
    RAMFOS_VECTOR ramfosBottomLine          ; C82Dh
    RAMFOS_VECTOR ramfosGetTop              ; C830h
    RAMFOS_VECTOR ramfosSetTop              ; C833h
    RAMFOS_VECTOR ramfosInput               ; C836h
    RAMFOS_VECTOR strToHex                  ; C839h
    RAMFOS_VECTOR ramfosSetCursor           ; C83Ch
    RAMFOS_VECTOR ramfosDeviceNoop          ; C83Fh
    RAMFOS_VECTOR ramfosDeviceNoop          ; C842h
    RAMFOS_VECTOR ramfosCancel              ; C845h
    RAMFOS_VECTOR beepTimer                 ; C848h
    RAMFOS_VECTOR ramfosSetColor            ; C84Bh
    RAMFOS_VECTOR ramfosClear               ; C84Eh
    RAMFOS_VECTOR ramfosOldBeep             ; C851h
    RAMFOS_VECTOR cmp_hl_de                 ; C854h
    RAMFOS_VECTOR ramfosDeviceNoop          ; C857h
    RAMFOS_VECTOR ramfosCycleTapeMode       ; C85Ah
