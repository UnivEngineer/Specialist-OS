; Редактор памяти, ввод HEX, пауза и HEX-дамп
; Порядок включения задан главным .asm; см. README.md.

txtSpaceQuestMark:  DB  "  ?",00H
txtNewLine:         DB  0AH,0DH,00H
txtHomeRight11Dot:  DB  0DH,18H,18H,18H,18H,18H,18H,18H,18H,18H,18H,18H,2EH,00H
txtFileQuestMark:   DB  "FILE? ",00H
                    DB  0AH,"FILE: ",00H

;----------------------------------------------------------------------------
; Директива M - редактирование блока памяти

Dir_M:  CALL  moveCursorUp
editMemoryByte:  CALL  printNewLine
        CALL  printHexWordHL
        LD    A,(HL)
        CALL  mon2_printHexByte
        CALL  printSpace
        CALL  mon2_getch
        CALL  RestartIfCls
        CP    1AH           ; кнопка вниз
        JP    Z,editNextByte
        CP    08H           ; кнопка влева
        JP    Z,editPreviousByte
        CALL  inputHexByteFromFirstChar
        LD    (HL),A
editNextByte:  INC   HL
        JP    editMemoryByte
editPreviousByte:  DEC   HL
        JP    editMemoryByte
inputHexByteFromFirstChar:  PUSH  BC
        JP    inputHexByteDigits
        PUSH  BC
        CALL  mon2_getch
inputHexByteDigits:  CALL  inputHexDigitEcho
        RLCA
        RLCA
        RLCA
        RLCA
        LD    B,A
        CALL  mon2_getch
        CALL  inputHexDigitEcho
        OR    B
        POP   BC
        RET

inputHexDigitEcho:  CP    20H           ; 32 ' '; [2]
        JP    C,RestartNoCls
        CP    3AH           ; 58 ':'
        JP    C,inputDecimalDigitEcho
        AND   5FH           ; 95 '_'
        CP    41H           ; 65 'A'
        JP    C,TypeErrorRestart
        CP    47H           ; 71 'G'
        JP    NC,TypeErrorRestart
        PUSH  AF
        LD    C,A
        CALL  mon2_printChar
        POP   AF
        SUB   37H           ; 55 '7'
        RET
inputDecimalDigitEcho:  OR    10H           ; 16
        PUSH  AF
        LD    C,A
        CALL  mon2_printChar
        POP   AF
        SUB   30H           ; 48 '0'
        RET

;----------------------------------------------------------------------------
; Печать знака вопроса и перезапуск

TypeErrorRestart:
        LD    HL,txtSpaceQuestMark
        CALL  bios_printStringOld
        JP    RestartNoCls

;----------------------------------------------------------------------------

inputCommandLineBody:  PUSH  BC
        PUSH  DE
        LD    DE,vars1.Directive
        CALL  inputLineCR
        POP   DE
        POP   BC
        POP   HL
        RET

;----------------------------------------------------------------------------
; Печать пробела

printSpace:
        LD    C,20H
        CALL  mon2_printChar
        RET

;----------------------------------------------------------------------------
; Ожидание нажатия клавиши СТР

WaitClsKey:
        CALL  mon2_keyScan
        CP    1FH
        RET   NZ
        CALL  mon2_getch
        ; продолжение в RestartIfCls

;----------------------------------------------------------------------------
; Перезапуск Монитора, если A == 1Fh

RestartIfCls:
        CP    1FH
        RET   NZ
        JP    RestartNoCls

;----------------------------------------------------------------------------
; Директива D - дамп блока памяти в HEX виде

Dir_D:  CALL  mon2_printHexWord
        LD    A,L
        AND   0FH
        LD    C,A
        CPL
        AND   0FH
        INC   A
        LD    B,A
        LD    A,C
        ADD   A,A
        ADD   A,A
        ADD   A,A
        ADD   A,C
        ADD   A,0FH
        LD    (bios_vars.cursorX),A
hexDumpByte:  LD    A,(HL)
        CALL  mon2_printHexByte
        LD    A,B
        CP    09H
        JP    NZ,hexDumpSpace
        LD    C,2DH         ; печать символа '-'
        CALL  mon2_printChar
        JP    hexDumpNext
hexDumpSpace:  CALL  printSpace
hexDumpNext:  CALL  bios_cmp_hl_de
        RET   Z
        DEC   B
        INC   HL
        JP    NZ,hexDumpByte
        CALL  WaitClsKey
        CALL  printNewLine
        JP    Dir_D
        LD    E,A
        RRCA
        RRCA
        RRCA
        RRCA
        CALL  nibbleToAscii
        LD    D,A
        LD    A,E
        CALL  nibbleToAscii
        LD    E,A
        RET
nibbleToAscii:  AND   0FH           ; 15; [2]
        CP    0AH           ; 10
        JP    C,nibbleAsciiDigit
        ADD   A,07H         ; 7
nibbleAsciiDigit:  ADD   A,30H         ; 48 '0'
        RET

;----------------------------------------------------------------------------

txtRegisters:
        DB    "A=",0
        DB    "F=",0
        DB    "B=",0
        DB    "C=",0
        DB    "D=",0
        DB    "E=",0
        DB    "H=",0
        DB    "L=",0
        DB    0AH,0DH," M(HL)=",0
        DB    "PC=",0
        DB    "SP=",0

;----------------------------------------------------------------------------

