; Примеры для SjASMPlus --i8080. Каждая метка demo — отдельный вход.
; directMain вызывается из MAIN, остальные примеры вызовов — из полного профиля MEM.
    INCLUDE "../../../../source/include/mxos.inc"
    ORG 02000h

directMain:
    ld c,'Q'
    call bios_printChar
    ret

demoPacket:
    ld bc,MXOS_BIOS_PRINT_CHAR
    ld de,packet
    call mx_call
    ret
packet:
    DW 0,0,'Q',0                ; HL, DE, BC, AF. Буфер доступен для записи.

demoRegisters:
    ld c,'Q'
    MXOS_CALL MXOS_BIOS_PRINT_CHAR
    ret

demoBatch:
    call mx_memInfo             ; A = FAR-код текущей страницы.
    ld hl,commands
    ld bc,commandsEnd-commands
    MXOS_CALL MXOS_BIOS_COMMAND_BATCH
    ret
commands:
    DB LOW MXOS_BIOS_PRINT_CHAR,5,4,'O',0
    DB LOW MXOS_BIOS_PRINT_CHAR,5,4,'K',0
commandsEnd:
    ASSERT commandsEnd-commands == 10

    MACRO CMP_HL_DE
        ld a,h
        cp d
        jp nz,.done
        ld a,l
        cp e
.done:
    ENDM

    MACRO SUB_HL_DE
        ld a,l
        sub e
        ld l,a
        ld a,h
        sbc a,d
        ld h,a
    ENDM

    MACRO DELAY_B
.loop:
        dec b
        jp nz,.loop
    ENDM

demoCompare:
    CMP_HL_DE
    ret
demoSubtract:
    SUB_HL_DE
    ret
demoDelay:
    DELAY_B
    ret
