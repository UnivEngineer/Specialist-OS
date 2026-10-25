; Поиск, сравнение, перенос, заполнение и текстовый дамп
; Порядок включения задан главным .asm; см. README.md.

;----------------------------------------------------------------------------
; Директива S — поиск последовательности из 1-4 байтов.

; Вход: HL = начало, DE = конец, C = первый байт; ещё до трёх байтов в строке.
; Выход: адреса совпадений с контекстом; регистры и флаги разрушены.
; Число байтов не проверяется, начало на конечном адресе не проверяется;
; сравнение образца может читать за концом диапазона, совпадения не перекрываются.
Dir_S:  PUSH  HL
        PUSH  BC
        LD    HL,INPUT_BUF
        LD    C,00H
countSearchCommas:  LD    A,(HL)
        CP    2CH           ; символ ','
        JP    NZ,countSearchNextChar
        INC   C             ; пропускаем ','
countSearchNextChar:  INC   HL
        CP    0DH           ; пеервод строки
        JP    NZ,countSearchCommas
        DEC   C
        LD    A,C
        LD    (vars1.SearchLength),A
        CP    01H
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
        LD    HL,vars1.SearchBytes34
        LD    (HL),E
        INC   HL
        LD    (HL),C
        POP   BC
        POP   DE
        POP   HL
storeFirstSearchByte:  LD    A,C
        LD    (vars1.SearchByte1),A
searchStart:  PUSH  HL
        LD    HL,vars1.SearchByte1
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
        LD    HL,txtHomeRight11Dot
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

;----------------------------------------------------------------------------

Dir_N:  LD    A,(HL)
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

;----------------------------------------------------------------------------
; Директива F - заполнить блок памяти байтом

Dir_F:  LD    (HL),C
        CALL  bios_cmp_hl_de
        RET   Z
        INC   HL
        JP    Dir_F

;----------------------------------------------------------------------------
; Директива T - копирование блока памяти на новый адрес
; Перекрывающиеся области памяти будут испорчены

Dir_T:  LD    A,(HL)
        LD    (BC),A
        CALL  bios_cmp_hl_de
        RET   Z
        INC   HL
        INC   BC
        JP    Dir_T

;----------------------------------------------------------------------------
; Директива C - сравнение двух блоков памяти

Dir_C:  LD    A,(BC)
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

;----------------------------------------------------------------------------
; Печать слова в HEX формате из HL, обрамляется пробелами

printHexWordHL:
        PUSH  BC
        CALL  printSpace
printHexWordHL_noSpace:
        LD    A,H
        CALL  mon2_printHexByte
        LD    A,L
        CALL  mon2_printHexByte
        CALL  printSpace
        POP   BC
        RET

;----------------------------------------------------------------------------
; Расчет и печать контрольной суммы
; вход:
;   hl = начальный адрес
;   de = конечный адрес

Dir_K:
        CALL  mon2_calcCS
        PUSH  BC
        POP   HL    ; hl = bc
        CALL  printHexWordHL
        RET

;----------------------------------------------------------------------------
; Директива L - дамп блока памяти в текстовом виде

Dir_L:  CALL  printHexWordHL
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

