;----------------------------------------------------------------------------
; MXOS - MON2.COM
;
; 2022-02-07 Дизассемблировано SpaceEngineer
;----------------------------------------------------------------------------

    INCLUDE "../include/mxos.inc"

; Используемые подпрограммы DOS.SYS:
; bios_keyScanOld        = 0C003h
; bios_drawCursorOld   = 0C006h
; bios_printCharOld    = 0C037h
; bios_beep_Old          = 0C170h
; bios_getchOld          = 0C337h
; bios_tapeReadOld     = 0C377h
; bios_tapeWriteOld    = 0C3D0h
; bios_cmp_hl_de         = 0C427h
; bios_printStringOld  = 0C438h
; bios_reboot            = 0C800h
; bios_getch             = 0C803h
; bios_printChar         = 0C809h
; bios_input             = 0C80Fh
; bios_keyCheck          = 0C812h
; bios_printHexByte    = 0C815h
; bios_printString     = 0C818h
; bios_keyScan           = 0C81Bh
; bios_getCursorPos    = 0C81Eh
; bios_tapeLoad          = 0C824h
; bios_tapeSave          = 0C827h
; bios_calcCS            = 0C82Ah
; bios_getMemTop         = 0C830h
; bios_setMemTop         = 0C833h
; bios_printer           = 0C836h
; bios_fileList          = 0C83Fh
; bios_fileGetSetDrive = 0C842h
; bios_fileCreate        = 0C845h
; bios_fileLoad          = 0C848h
; bios_fileLoadInfo    = 0C851h
; bios_fileGetSetAddr  = 0C854h
; bios_fileGetSetAttr  = 0C857h
; bios_fileNamePrepare = 0C85Ah
; bios_fileLoad2         = 0C866h

; Используемые переменные DOS.SYS:
; bios_vars.tapeError  = 8FE1h
; bios_vars.tapeAddr   = 8FE3h
; bios_vars.cursorCfg  = 8FE9h
; bios_vars.koi8         = 8FEAh
; bios_vars.cursorX    = 8FFCh

; Собственные переменные
    STRUCT MON2_VARS_1
Directive:     DS    1
FirstParam:     DS    2
SecondParam:     DS    2
ThirdParam:     DS    2
ParseTail:     DS    2
SearchLength:     DS    1
SearchByte1:     DS    1
SearchByte2:     DS    1
SearchBytes34:     DS    2
SearchCursor:     DS    2
SearchRemainingCopy:     DS    1
Reserved91:     DS    4
InputCharacter:     DS    1
Reserved96:     DS    1
BreakpointAddress:     DS    2
BreakpointOriginalByte:     DS    1
SavedRst7Vector:     DS    3
SavedHL:     DS    2
SavedDE:     DS    2
SavedBC:     DS    2
SavedAF:     DS    2
SavedSP:     DS    2
ReservedA7:     DS    4
LegacyMemoryTop:     DS    2
    ENDS

    STRUCT MON2_VARS_2
LegacyIoReserved:     DS    5
LegacyIoData:     DS    1
LegacyIoStatus:     DS    1
LegacyIoControl:     DS    1
    ENDS

    STRUCT MON2_VARS_3
NameAndAttributes:     DS    10
LoadAddress:     DS    2
Size:     DS    2
    ENDS

; Адреса блоков переменных
vars1 MON2_VARS_1 = 08F80H
vars2 MON2_VARS_2 = 0F0E0H
vars3 MON2_VARS_3 = 0F900H


; Начало программы
    ORG   0F100h

        LD    C,1FH         ; 31
        CALL  bios_printChar
RestartNoCls:   LD    SP,0FFBFH   ; 65471; [7]
        LD    HL,mon2_restart    ; 63488
        LD    (bios_vars.tapeError),HL
        LD    HL,7EFFH    ; 32511
        LD    (vars1.LegacyMemoryTop),HL
        CALL  MonitorMain
        JP    RestartNoCls
MonitorMain:   LD    HL,txtNewLine             ; 62970
        CALL  bios_printStringOld
        LD    E,02H         ; 2
        CALL  bios_fileGetSetDrive
        ADD   A,41H         ; 65 'A'
        LD    C,A
        CALL  bios_printChar
        LD    C,3EH         ; 62 '>'
        CALL  bios_printChar
        CALL  ClearInputBuf
        CALL  inputCommandLine
        PUSH  HL
        PUSH  DE
        PUSH  BC
        LD    E,02H         ; 2
        CALL  bios_fileGetSetDrive
        PUSH  AF
        LD    HL,8F60H    ; 36704
        LD    DE,vars3.NameAndAttributes   ; 63744
        CALL  bios_fileNamePrepare
        LD    E,02H         ; 2
        CALL  bios_fileGetSetDrive
        LD    E,A
        POP   AF
        CP    E
        POP   BC
        POP   DE
        POP   HL
        RET   NZ
        CALL  printNewLine
        CALL  Tokenizer
        CP    44H           ; 68 'D'
        JP    Z,Dir_D
        CP    4DH           ; 77 'M'
        JP    Z,Dir_M
        CP    4CH           ; 76 'L'
        JP    Z,Dir_L
        CP    4BH           ; 75 'K'
        JP    Z,Dir_K
        CP    54H           ; 84 'T'
        JP    Z,Dir_T
        CP    58H           ; 88 'X'
        JP    Z,Dir_X
        CP    57H           ; 87 'W'
        JP    Z,mon2_tapeSave
        CP    52H           ; 82 'R'
        JP    Z,Dir_R
        CP    43H           ; 67 'C'
        JP    Z,Dir_C
        CP    48H           ; 72 'H'
        JP    Z,Dir_H
        CP    4EH           ; 78 'N'
        JP    Z,Dir_N
        CP    47H           ; 71 'G'
        JP    Z,Dir_G
        CP    46H           ; 70 'F'
        JP    Z,Dir_F
        CP    53H           ; 83 'S'
        JP    Z,Dir_S
        CP    4AH           ; 74 'J'
        JP    Z,Dir_J
        CP    3FH           ; 63 '?'
        JP    Z,Dir_DirDisk
        CP    42H           ; 66 'B'
        JP    Z,Dir_B
        CP    41H           ; 65 'A'
        JP    Z,Dir_A
        CP    56H           ; 86 'V'
        JP    Z,Dir_V
        CP    55H           ; 85 'U'
        JP    Z,Dir_U
        CP    59H           ; 89 'Y'
        JP    Z,Dir_Y
        CP    51H           ; 81 'Q'
        JP    Z,Dir_Q
        JP    TypeErrorRestart
Dir_J:   CALL  bios_keyCheck             ; [2]
        INC   A
        JP    Z,bios_reboot
        JP    Dir_J
ClearInputBuf:   LD    HL,8F60H    ; 36704
        LD    (HL),00H
        INC   HL
        LD    (HL),00H
        RET
Dir_DirDisk:   LD    HL,vars3.NameAndAttributes   ; 63744
        CALL  bios_fileList
DirDiskLoop:   LD    A,(HL)
        INC   A
        RET   Z
        CALL  bios_keyCheck
        CP    1FH           ; 31
        CALL  Z,bios_getch
        CP    1FH           ; 31
        RET   Z
        LD    B,06H         ; 6
        CALL  printStringB
        LD    C,2EH         ; 46 '.'
        CALL  bios_printChar
        LD    B,03H         ; 3
        CALL  printStringB
        CALL  printSpace
        INC   HL
        LD    E,(HL)
        INC   HL
        LD    D,(HL)
        PUSH  DE
        INC   HL
        LD    E,(HL)
        INC   HL
        LD    D,(HL)
        EX    (SP),HL
        CALL  printHexWordHL
        ADD   HL,DE
        CALL  printHexWordHL
        POP   HL
        LD    A,L
        AND   0F0H          ; 240
        LD    L,A
        LD    DE,0010H    ; 16
        ADD   HL,DE
        CALL  printNewLine
        JP    DirDiskLoop
printStringB:   LD    C,(HL)                ; [3]
        CALL  bios_printChar
        INC   HL
        DEC   B
        JP    NZ,printStringB
        RET
Dir_B:   LD    (vars3.LoadAddress),HL
        EX    DE,HL
        LD    A,D
        CPL
        LD    D,A
        LD    A,E
        CPL
        LD    E,A
        INC   DE
        ADD   HL,DE
        LD    (vars3.Size),HL
        CALL  EnterFileName
        CALL  bios_fileCreate
        LD    HL,txtSmallDisk             ; 62041
        CALL  C,printCreateError
        RET
printCreateError:   OR    A
        JP    Z,printDirectoryFull
        CALL  bios_printString
        RET
printDirectoryFull:   LD    HL,txtSmallDir             ; 62054
        CALL  bios_printString
        RET

        ; Русский текст в кодировке КОИ-8
txtSmallDisk:   DB    0AH,0DH,"mal disk !",00H
txtSmallDir:   DB    0AH,0DH,"mal DIR !", 00H
txtNoFile:   DB    0AH,0DH,"net fajla ",00H

Dir_A:   PUSH  HL
        CALL  EnterFileName
        POP   DE
        LD    C,01H         ; 1
        CALL  bios_fileGetSetAddr
        LD    HL,txtNoFile             ; 62066
        CALL  C,bios_printString
        RET
Dir_V:   PUSH  HL
        CALL  EnterFileName
        POP   DE
        PUSH  DE
        CALL  bios_fileLoad2
        PUSH  AF
        LD    HL,txtNoFile             ; 62066
        CALL  C,bios_printString
        POP   AF
        POP   DE
        RET   C
        PUSH  DE
        LD    HL,vars3.NameAndAttributes   ; 63744
        CALL  bios_fileLoadInfo
        LD    HL,(vars3.Size)
        EX    DE,HL
        POP   HL
        CALL  printHexWordHL
        ADD   HL,DE
        CALL  printHexWordHL
        RET
Dir_U:   CALL  EnterFileName
        CALL  bios_fileLoad
        PUSH  AF
        LD    HL,txtNoFile             ; 62066
        CALL  C,bios_printString
        CALL  printSpace
        POP   AF
        RET   C
        LD    HL,vars3.NameAndAttributes   ; 63744
        CALL  bios_fileLoadInfo
        LD    HL,(vars3.LoadAddress)
        PUSH  HL
        CALL  printHexWordHL
        POP   DE
        LD    HL,(vars3.Size)
        ADD   HL,DE
        CALL  printHexWordHL
        RET
Dir_Y:  PUSH  HL
        CALL  EnterFileName
        LD    C,01H         ; 1
        EX    (SP),HL
        LD    A,L
        POP   HL
        CALL  bios_fileGetSetAttr
        LD    HL,txtNoFile             ; 62066
        CALL  C,bios_printString
        RET
Dir_Q:  CALL  EnterFileName
        LD    C,02H         ; 2
        CALL  bios_fileGetSetAttr
        PUSH  AF
        LD    HL,txtNoFile             ; 62066
        CALL  C,bios_printString
        CALL  printSpace
        POP   AF
        CALL  NC,bios_printHexByte
        RET
EnterFileName:   LD    HL,txtFileQuestMark             ; 62987; [6]
        CALL  bios_printString
        LD    HL,8F60H    ; 36704
        LD    DE,8F6DH    ; 36717
        CALL  bios_input
        LD    DE,vars3.NameAndAttributes   ; 63744
        CALL  bios_fileNamePrepare
        EX    DE,HL
        RET
Dir_R:  CALL  mon2_tapeLoad
        PUSH  BC
        PUSH  DE
        PUSH  HL
        CALL  bios_calcCS
        POP   HL
        CALL  printHexWordHL
        POP   HL
        CALL  printHexWordHL
        POP   HL
        LD    D,B
        LD    E,C
        CALL  bios_cmp_hl_de
        JP    NZ,printChecksumMismatch
        CALL  printHexWordHL
        RET
printChecksumMismatch:  CALL  printSpace
        LD    C,3FH         ; 63 '?'
        CALL  bios_printChar
        RET
drawSearchMarker: PUSH  AF
        LD    A,(bios_vars.cursorCfg)
        PUSH  AF
        LD    A,11H         ; 17
        JP    drawSearchMarkerBody
        NOP
        NOP
Dir_H:  PUSH  HL
        ADD   HL,DE
        CALL  printHexWordHL
        CALL  printSpace
        LD    A,E
        CPL
        LD    E,A
        LD    A,D
        CPL
        LD    D,A
        INC   DE
        POP   HL
        ADD   HL,DE
        CALL  printHexWordHL
        RET
Dir_X:  LD    HL,vars1.SavedAF+1 ; 36772
        LD    DE,txtRegisters             ; 63244
        LD    C,04H         ; 4
printRegisterPairsLoop:  PUSH  BC
        CALL  printSavedRegisterPair
        POP   BC
        DEC   C
        JP    NZ,printRegisterPairsLoop
        CALL  printRegisterName
        LD    HL,(vars1.SavedHL)
        CALL  printHexWordHL
        CALL  printRegisterName
        LD    HL,(vars1.BreakpointAddress)
        CALL  printHexWordHL
        CALL  printRegisterName
        LD    HL,(vars1.SavedSP)
        CALL  printHexWordHL
        RET
setMIfKeyPressed:  CALL  bios_keyScan
        INC   A
        RET   Z
        LD    (HL),0FFH   ; 255
        RET
printSavedRegisterPair:   LD    B,(HL)
        DEC   HL
        LD    C,(HL)
        DEC   HL
        PUSH  BC
        CALL  printRegisterName
        LD    A,B
        CALL  mon2_printHexByte
        CALL  printRegisterName
        POP   BC
        LD    A,C
        CALL  mon2_printHexByte
        RET
printRegisterName:   EX    DE,HL         ; [5]
        PUSH  BC
        CALL  printSpace
        CALL  bios_printStringOld
        INC   HL
        POP   BC
        EX    DE,HL
        RET
Dir_G:  LD    A,E
        OR    A
        JP    NZ,installBreakpoint
        LD    A,D
        OR    A
        JP    Z,executeAddress
installBreakpoint:  PUSH  HL
        EX    DE,HL
        LD    (vars1.BreakpointAddress),HL
        LD    A,(HL)
        LD    (HL),0FFH   ; 255
        LD    (vars1.BreakpointOriginalByte),A
        LD    HL,0038H    ; 56
        LD    DE,vars1.SavedRst7Vector   ; 36762
        LD    BC,breakpointTrap             ; 62455
        CALL  saveVectorByte
        LD    (HL),0C3H   ; 195
        CALL  saveNextVectorByte
        CALL  saveNextVectorByte
        LD    (HL),B
        DEC   HL
        LD    (HL),C
        POP   HL
executeAddress:  CALL  callAddressHL
        JP    RestartNoCls
callAddressHL:   JP    (HL)
saveNextVectorByte:  INC   HL            ; [4]
        INC   DE
saveVectorByte:  LD    A,(HL)                ; [2]
        LD    (DE),A
        RET
breakpointTrap:   LD    (vars1.SavedHL),HL
        EX    DE,HL
        LD    (vars1.SavedDE),HL
        PUSH  BC
        POP   HL
        LD    (vars1.SavedBC),HL
        PUSH  AF
        POP   HL
        LD    (vars1.SavedAF),HL
        LD    HL,0000H
        ADD   HL,SP
        INC   HL
        INC   HL
        LD    (vars1.SavedSP),HL
        LD    HL,(vars1.BreakpointAddress)
        LD    A,(vars1.BreakpointOriginalByte)
        LD    (HL),A
        LD    HL,vars1.SavedRst7Vector   ; 36762
        LD    DE,0038H    ; 56
        CALL  saveVectorByte
        CALL  saveNextVectorByte
        CALL  saveNextVectorByte
        JP    RestartNoCls
Dir_S:  PUSH  HL
        PUSH  BC
        LD    HL,8F60H    ; 36704
        LD    C,00H
countSearchCommas:  LD    A,(HL)
        CP    2CH           ; 44 ','
        JP    NZ,countSearchNextChar
        INC   C
countSearchNextChar:  INC   HL
        CP    0DH           ; 13
        JP    NZ,countSearchCommas
        DEC   C
        LD    A,C
        LD    (vars1.SearchLength),A
        CP    01H           ; 1
        POP   BC
        POP   HL
        JP    Z,storeFirstSearchByte
        PUSH  HL
        PUSH  DE
        PUSH  BC
        LD    HL,(vars1.ParseTail)
        LD    C,L
        LD    B,H
        CALL  parseThreeParameters
        LD    A,L
        LD    (vars1.SearchByte2),A
        LD    HL,vars1.SearchBytes34   ; 36748
        LD    (HL),E
        INC   HL
        LD    (HL),C
        POP   BC
        POP   DE
        POP   HL
storeFirstSearchByte:  LD    A,C
        LD    (vars1.SearchByte1),A
searchStart:  PUSH  HL            ; [2]
        LD    HL,vars1.SearchByte1   ; 36746
        LD    (vars1.SearchCursor),HL
        POP   HL
        CALL  bios_cmp_hl_de
        RET   Z
        CALL  WaitClsKey
        LD    A,(vars1.SearchLength)
        LD    (vars1.SearchRemainingCopy),A
        LD    B,A
searchCompareByte:  LD    C,(HL)
        PUSH  HL
        LD    HL,(vars1.SearchCursor)
        LD    A,(HL)
        CP    C
        JP    NZ,searchNextCandidate
        INC   HL
        LD    (vars1.SearchCursor),HL
        POP   HL
        INC   HL
        DEC   B
        JP    NZ,searchCompareByte
        PUSH  HL
        PUSH  DE
        PUSH  BC
        PUSH  AF
        LD    A,(vars1.SearchLength)
rewindSearchMatch:  DEC   HL
        DEC   A
        JP    NZ,rewindSearchMatch
        CALL  printHexWordHL
        DEC   HL
        DEC   HL
        LD    A,(vars1.SearchLength)
        ADD   A,04H         ; 4
        LD    B,A
printSearchContext:  CALL  printMemoryByte
        INC   HL
        DEC   B
        JP    NZ,printSearchContext
        LD    HL,txtHomeRight11Dot             ; 62973
        CALL  mon2_printString
        LD    A,(vars1.SearchLength)
        LD    B,A
        ADD   A,B
        ADD   A,B
        DEC   A
        LD    E,A
        LD    C,18H         ; 24
drawSearchMarkerLoop:  CALL  drawSearchMarker
        DEC   E
        JP    NZ,drawSearchMarkerLoop
        CALL  printNewLine
        POP   AF
        POP   BC
        POP   DE
        POP   HL
        JP    searchStart
searchNextCandidate:  POP   HL
        INC   HL
        JP    searchStart
printMemoryByte:  LD    A,(HL)
        PUSH  BC
        CALL  mon2_printHexByte
        CALL  printSpace
        POP   BC
        RET
Dir_N:  LD    A,(HL)                ; [2]
        CP    C
        JP    Z,nextUnequalByte
        CALL  printHexWordHL
        LD    A,(HL)
        PUSH  BC
        CALL  mon2_printHexByte
        CALL  WaitClsKey
        CALL  printNewLine
        POP   BC
nextUnequalByte:  CALL  bios_cmp_hl_de
        RET   Z
        INC   HL
        JP    Dir_N
Dir_F:  LD    (HL),C                ; [2]
        CALL  bios_cmp_hl_de
        RET   Z
        INC   HL
        JP    Dir_F
Dir_T:  LD    A,(HL)                ; [2]
        LD    (BC),A
        CALL  bios_cmp_hl_de
        RET   Z
        INC   HL
        INC   BC
        JP    Dir_T
Dir_C:  LD    A,(BC)                ; [2]
        CP    (HL)
        JP    Z,compareNextByte
        PUSH  BC
        CALL  printHexWordHL
        LD    A,(HL)
        CALL  mon2_printHexByte
        CALL  printSpace
        POP   BC
        PUSH  BC
        LD    A,(BC)
        CALL  mon2_printHexByte
        CALL  WaitClsKey
        CALL  printNewLine
        POP   BC
compareNextByte:  CALL  bios_cmp_hl_de
        RET   Z
        INC   HL
        INC   BC
        JP    Dir_C
printHexWordHL:  PUSH  BC            ; [20]
        CALL  printSpace
printHexWordHL_noSpace:  LD    A,H
        CALL  mon2_printHexByte
        LD    A,L
        CALL  mon2_printHexByte
        CALL  printSpace
        POP   BC
        RET
Dir_K:  CALL  mon2_calcCS
        PUSH  BC
        POP   HL
        CALL  printHexWordHL
        RET
Dir_L:  CALL  printHexWordHL         ; [2]
        LD    B,10H         ; 16
textDumpByte:  LD    C,(HL)
        LD    A,(vars1.ThirdParam)
        OR    A
        JP    Z,textDumpFilter
        AND   C
        LD    C,A
textDumpFilter:  LD    A,C
        CP    20H           ; 32 ' '
        JP    C,textDumpNonprintable
        CP    7FH           ; 127
        JP    C,textDumpPrint
textDumpNonprintable:  LD    C,2EH         ; 46 '.'
textDumpPrint:  CALL  mon2_printChar
        CALL  printSpace
        CALL  bios_cmp_hl_de
        RET   Z
        DEC   B
        INC   HL
        JP    NZ,textDumpByte
        CALL  WaitClsKey
        CALL  printNewLine
        JP    Dir_L
Tokenizer:  LD    BC,8F60H    ; 36704
        LD    A,(BC)
        LD    (vars1.Directive),A
        INC   BC
parseThreeParameters:  CALL  ReadHexWord
        LD    (vars1.FirstParam),HL
        CALL  ReadHexWord
        LD    (vars1.SecondParam),HL
        CALL  ReadHexWord
        LD    (vars1.ThirdParam),HL
        LD    L,C
        LD    H,B
        LD    (vars1.ParseTail),HL
        LD    HL,(vars1.ThirdParam)
        LD    C,L
        LD    B,H
        LD    HL,(vars1.SecondParam)
        EX    DE,HL
        LD    HL,(vars1.FirstParam)
        LD    A,(vars1.Directive)
        RET
ReadHexWord:  LD    HL,0000H    ; [3]
readHexWordDigit:  LD    A,(BC)
        CP    0DH           ; 13
        RET   Z
        CP    2CH           ; 44 ','
        JP    Z,skipParameterComma
        CALL  hexDigitValue
        ADD   HL,HL
        ADD   HL,HL
        ADD   HL,HL
        ADD   HL,HL
        ADD   A,L
        LD    L,A
        INC   BC
        JP    readHexWordDigit
skipParameterComma:  INC   BC
        RET
hexDigitValue:  SUB   30H           ; 48 '0'
        CP    0AH           ; 10
        RET   C
        SUB   11H           ; 17
        CP    06H           ; 6
        JP    NC,TypeErrorRestart
        ADD   A,0AH         ; 10
        RET
moveCursorUp:  PUSH  BC
        LD    C,19H         ; 25
        CALL  mon2_printChar
        POP   BC
        RET
inputCommandLine:  PUSH  HL
        LD    HL,8F60H    ; 36704
        JP    inputCommandLineBody
printNewLine:  LD    C,0AH         ; 10; [8]
        CALL  mon2_printChar
        LD    C,0DH         ; 13
        CALL  mon2_printChar
        RET

        ; Русский текст в кодировке КОИ-8
txtSpaceQuestMark:   DB    "  ?",00H
txtNewLine:   DB    0AH,0DH,00H
txtHomeRight11Dot:   DB    0DH,18H,18H,18H,18H,18H,18H,18H,18H,18H,18H,18H,2EH,00H
txtFileQuestMark:   DB    "fajl ? ",00H
        DB    0AH,"fajl: ",00H

Dir_M:  CALL  moveCursorUp
editMemoryByte:  CALL  printNewLine         ; [2]
        CALL  printHexWordHL
        LD    A,(HL)
        CALL  mon2_printHexByte
        CALL  printSpace
        CALL  mon2_getch
        CALL  RestartIfCls
        CP    1AH           ; 26
        JP    Z,editNextByte
        CP    08H           ; 8
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
TypeErrorRestart:  LD    HL,txtSpaceQuestMark             ; 62966; [4]
        CALL  bios_printStringOld
        JP    RestartNoCls
inputCommandLineBody:  PUSH  BC
        PUSH  DE
        LD    DE,vars1.Directive   ; 36736
        CALL  inputLineCR
        POP   DE
        POP   BC
        POP   HL
        RET
printSpace:  LD    C,20H         ; 32 ' '; [13]
        CALL  mon2_printChar
        RET
WaitClsKey:  CALL  mon2_keyScan         ; [5]
        CP    1FH           ; 31
        RET   NZ
        CALL  mon2_getch
RestartIfCls:  CP    1FH           ; 31
        RET   NZ
        JP    RestartNoCls
Dir_D:  CALL  mon2_printHexWord         ; [2]
        LD    A,L
        AND   0FH           ; 15
        LD    C,A
        CPL
        AND   0FH           ; 15
        INC   A
        LD    B,A
        LD    A,C
        ADD   A,A
        ADD   A,A
        ADD   A,A
        ADD   A,C
        ADD   A,0FH         ; 15
        LD    (bios_vars.cursorX),A
hexDumpByte:  LD    A,(HL)
        CALL  mon2_printHexByte
        LD    A,B
        CP    09H           ; 9
        JP    NZ,hexDumpSpace
        LD    C,2DH         ; 45 '-'
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

txtRegisters:   DB    "A=",0
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
inputLineCR:  LD    B,H
        LD    C,L
inputLineKey:  CALL  mon2_getch         ; [4]
        CP    08H           ; 8
        JP    Z,inputLineBackspace
        CP    0DH           ; 13
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

;----------------------------------------------------------------------------
; Таблица переходов F800h
;----------------------------------------------------------------------------
    ORG_PAD0 0F800h


mon2_restart:  JP    RestartNoCls                  ; F800
mon2_getch:  JP    bios_getchOld         ; F803
        JP    bios_tapeReadOld      ; F806
mon2_printChar:  JP    bios_printCharOld     ; F809
        JP    bios_tapeWriteOld     ; F80C
        JP    bios_printer          ; F80F
        JP    setMIfKeyPressed                 ; F812
mon2_printHexByte:  JP    bios_printHexByte     ; F815
mon2_printString:  JP    bios_printStringOld   ; F818
mon2_keyScan:  JP    bios_keyScanOld       ; F81B
        JP    bios_getCursorPos     ; F81E
        RET                         ; F821 - не используется
        NOP
        NOP
mon2_tapeLoad:  JP    bios_tapeLoad         ; F824
mon2_tapeSave:  JP    bios_tapeSave         ; F827
mon2_calcCS:  JP    bios_calcCS           ; F82A
        JP    bios_beep_Old         ; F82D
        JP    bios_getMemTop        ; F830
        JP    bios_setMemTop        ; F833
mon2_printHexWord:  PUSH  BC                    ; F836
        JP    printHexWordHL_noSpace

    END
