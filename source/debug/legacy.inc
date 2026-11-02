; Остатки старого обмена и общий ввод строки
; Порядок включения задан главным .asm; см. README.md.

legacyOutputByte:  PUSH  AF            ; [2]
legacyWaitReady:  LD    A,(vars2.LegacyIoStatus)
        AND   0CH           ; 12
        JP    NZ,legacyWaitReady
        LD    A,C
        LD    (vars2.LegacyIoData),A
        LD    A,(vars2.LegacyIoStatus)
        OR    20H           ; 32 ' '
        LD    (vars2.LegacyIoStatus),A
legacyWaitAck:  LD    A,(vars2.LegacyIoStatus)
        AND   04H           ; 4
        JP    Z,legacyWaitAck
        XOR   A
        LD    (vars2.LegacyIoStatus),A
        POP   AF
        RET
        PUSH  HL
legacyOutputStringLoop:  LD    A,(HL)
        OR    A
        JP    Z,legacyOutputStringReturn
        LD    C,A
        CALL  legacyOutputByte
        INC   HL
        JP    legacyOutputStringLoop
legacyOutputStringReturn:  POP   HL
        RET
        PUSH  AF
        LD    A,91H         ; 145
        LD    (vars2.LegacyIoControl),A
        LD    A,(vars2.LegacyIoStatus)
        OR    10H           ; 16
        LD    (vars2.LegacyIoStatus),A
        AND   0EFH          ; 239
        LD    (vars2.LegacyIoStatus),A
        LD    C,0FH         ; 15
        CALL  legacyOutputByte
        POP   AF
        RET
drawSearchMarkerBody:  LD    (bios_vars.cursorCfg),A
        CALL  bios_drawCursorOld
        POP   AF
        LD    (bios_vars.cursorCfg),A
        LD    C,18H         ; 24
        POP   AF
        JP    mon2_printChar
;----------------------------------------------------------------------------

; Вход: HL = буфер, DE = адрес предела. Выход: строка с завершающим CR.
; HL указывает на CR; BC = исходный HL. AF и флаги разрушены, DE сохранён.
; Предел относится к символам: завершающий CR пишется даже по адресу DE.
inputLineCR:  LD    B,H
        LD    C,L
inputLineKey:  CALL  mon2_getch
        CP    08H           ; клавиша влево
        JP    Z,inputLineBackspace
        CP    0DH           ; клавиша Enter
        JP    Z,inputLineEnter
        LD    (vars1.InputCharacter),A
        CALL  bios_cmp_hl_de
        JP    Z,inputLineKey
        LD    A,(vars1.InputCharacter)
        LD    (HL),A
        PUSH  BC
        LD    C,A
        CALL  mon2_printChar
        POP   BC
        INC   HL
        JP    inputLineKey

;----------------------------------------------------------------------------

inputLineBackspace:  LD    A,H
        CP    B
        JP    NZ,inputLineErase
        LD    A,L
        CP    C
        JP    Z,inputLineKey
inputLineErase:  DEC   HL
        PUSH  BC
        LD    C,08H         ; 8
        CALL  mon2_printChar
        LD    C,20H         ; 32 ' '
        CALL  mon2_printChar
        LD    C,08H         ; 8
        CALL  mon2_printChar
        POP   BC
        JP    inputLineKey
inputLineEnter:
        LD    (HL),A
        RET
        PUSH  HL
        LD    HL,0FFE3H   ; 65507
        LD    (HL),0DH    ; 13
        LD    (HL),0CH    ; 12
        POP   HL
        RET

