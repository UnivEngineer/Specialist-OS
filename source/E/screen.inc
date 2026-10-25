; Примитивы памяти, курсор, экран и строка состояния
; Порядок включения задан главным .asm; см. README.md.

testShiftKey:  LD    A,(IO_KEYB_B) ; [3]
        AND   02H           ; 2
        RET
; Вход: BC = источник, HL = приёмник, DE = конец приёмника (исключая).
; Выход: HL = DE, BC увеличен; DE сохранён, AF и флаги разрушены.
; Нулевая длина не поддерживается; копирование всегда выполняет первый байт.
copyUntilDE:  LD    A,(BC)        ; [5]
        LD    (HL),A
        INC   HL
        INC   BC
        CALL  compareHLDE
        JP    NZ,copyUntilDE
        RET
; Вход: HL, DE. Выход: флаги сравнения HL с DE; пары сохранены, A разрушен.
compareHLDE:  LD    A,H           ; [34]
        CP    D
        RET   NZ
        LD    A,L
        CP    E
        RET
subtractHLFromDE:  LD    A,E           ; [9]
        SUB   L
        LD    E,A
        LD    A,D
        SBC   A,H
        LD    D,A
        RET

        ; Звуковой сигнал 4 раза через п/п печати символа 07h
Beep4:
        LD    BC,0407H      ; b = счетчик цикла, c = код символа

        ; Печать b раз символа с кодом c
TypeCharCtimesB:
        CALL  bios_printChar
        DEC   B
        JP    NZ,TypeCharCtimesB
        RET

; Вход: HL = текст. Выход: HL после 0Dh либо на 0FFh; A = этот разделитель.
; Разрушает AF и флаги, остальные регистры сохраняет.
nextLine:  LD    A,(HL)                ; [11]
        CP    0FFH          ; 255
        RET   Z
        CP    0DH           ; 13
        INC   HL
        JP    NZ,nextLine
        RET
saveCursor:  PUSH  HL            ; [7]
        LD    HL,(bios_vars.cursorY)
        ; САМОМОДИФИКАЦИЯ: сохранение курсора в операнде LD HL,nn.
        LD    (smc_restoreCursor+1),HL
        POP   HL
        RET
restoreCursor:  PUSH  HL            ; [7]
smc_restoreCursor:   LD    HL,0000H
        LD    (bios_vars.cursorY),HL
        POP   HL
        RET
resumeAtTextPointer: CALL  resetSessionStack
        CALL  previousLine
        LD    (v_currentLine),HL
        LD    (v_topLine),HL
        CALL  recountCurrentLine
        CALL  drawStatusBar
        CALL  drawPage
        CALL  bios_getch
        PUSH  AF
        LD    HL,06F9H    ; 1785
        LD    (bios_vars.cursorY),HL
        LD    BC,3020H    ; 12320
        CALL  TypeCharCtimesB
        CALL  finishPageAndHome
        POP   AF
        LD    HL,editorStatusLoop    ; 53535
        PUSH  HL
        JP    keyInput2
scrollTextUp:  LD    H,90H         ; 144
        LD    D,H
scrollUpColumn: LD    C,39H         ; 57 '9'
        LD    L,01H         ; 1
        LD    E,0BH         ; 11
scrollUpFourBytes: LD    A,(DE)
        LD    (HL),A
        INC   HL
        INC   DE
        LD    A,(DE)
        LD    (HL),A
        INC   HL
        INC   DE
        LD    A,(DE)
        LD    (HL),A
        INC   HL
        INC   DE
        LD    A,(DE)
        LD    (HL),A
        INC   HL
        INC   DE
        DEC   C
        JP    NZ,scrollUpFourBytes
        LD    C,0AH         ; 10
        XOR   A
clearScrollUpBottom: LD    (HL),A
        INC   HL
        DEC   C
        JP    NZ,clearScrollUpBottom
        LD    A,H
        CP    0BFH          ; 191
        RET   Z
        INC   H
        INC   D
        JP    scrollUpColumn
insertScreenRow:  LD    A,(bios_vars.cursorY)  ; [2]
        LD    L,A
        LD    A,0EEH                ; 238
        SUB   L
        OR    A
        rra
        ; САМОМОДИФИКАЦИЯ: число пар байтов прокрутки в операнде LD A,n.
        LD    (smc_scrollPairCount+1),A
        LD    H,0BFH                ; 191
        LD    D,H
        LD    B,30H         ; 48 '0'
scrollDownColumn: LD    L,0EFH                ; 239
        LD    E,0E5H                ; 229
smc_scrollPairCount:   LD    A,00H
        LD    C,A
        OR    A
        JP    Z,clearInsertedRow
scrollDownPair: LD    A,(DE)
        LD    (HL),A
        DEC   HL
        DEC   DE
        LD    A,(DE)
        LD    (HL),A
        DEC   HL
        DEC   DE
        DEC   C
        JP    NZ,scrollDownPair
clearInsertedRow: LD    C,09H         ; 9
        XOR   A
clearInsertedRowByte: LD    (HL),A
        DEC   HL
        DEC   DE
        DEC   C
        JP    NZ,clearInsertedRowByte
        DEC   H
        LD    D,H
        DEC   B
        JP    NZ,scrollDownColumn
        RET

        ; Обновление стаус бара
updateEditMode:  PUSH  HL
        LD    HL,(bios_vars.cursorY)
        PUSH  HL
        CALL  enableInverse
        LD    HL,9DF9H    ; 40441
        LD    (bios_vars.cursorY),HL
        LD    A,(v_insertMode)
        OR    A
        LD    HL,txtIns
        JP    NZ,printEditMode
        LD    HL,txtOvr
printEditMode: CALL  bios_printString
        CALL  disableInverse
        POP   HL
        LD    (bios_vars.cursorY),HL
        POP   HL
        RET

nextCurrentLine:  LD    HL,(v_currentLine)   ; [5]
        CALL  nextLine
        LD    A,(HL)
        INC   A
        RET   Z
        LD    (v_currentLine),HL
        LD    HL,(v_lineNumber)
        INC   HL
        LD    (v_lineNumber),HL
        XOR   A
        INC   A
        RET
previousCurrentLine:  LD    HL,(v_currentLine)   ; [3]
        CALL  previousLine
        RET   Z
        LD    (v_currentLine),HL
        LD    HL,(v_lineNumber)
        DEC   HL
        LD    (v_lineNumber),HL
        RET
updateLineNumber:  PUSH  DE            ; [3]
        PUSH  BC
        PUSH  HL
        LD    HL,(bios_vars.cursorY)
        PUSH  HL
        CALL  enableInverse
        LD    HL,67F9H    ; 26617
        LD    (bios_vars.cursorY),HL
        LD    HL,(v_lineNumber)
        LD    B,20H         ; 32 ' '
        LD    DE,0FC18H   ; 64536
        CALL  printDecimalDigit
        LD    DE,03E8H    ; 1000
        ADD   HL,DE
        LD    DE,0FF9CH   ; 65436
        CALL  printDecimalDigit
        LD    DE,0064H    ; 100
        ADD   HL,DE
        LD    DE,0FFF6H   ; 65526
        CALL  printDecimalDigit
        LD    A,L
        ADD   A,3AH         ; 58 ':'
        LD    C,A
        CALL  bios_printChar
        CALL  disableInverse
        POP   HL
        LD    (bios_vars.cursorY),HL
        POP   HL
        POP   BC
        POP   DE
        RET
printDecimalDigit:  LD    C,2FH         ; 47 '/'; [4]
decimalDigitLoop: INC   C
        ADD   HL,DE
        JP    C,decimalDigitLoop
        LD    A,C
        CP    30H           ; 48 '0'
        JP    Z,decimalDigitPrint
        LD    B,0FFH                ; 255
decimalDigitPrint: AND   B
        LD    C,A
        JP    bios_printChar
recountCurrentLine:  LD    HL,(v_currentLine)   ; [3]
        EX    DE,HL
        LD    HL,(v_textBeginLo)
        LD    (v_currentLine),HL
        LD    HL,0001H    ; 1
        LD    (v_lineNumber),HL
recountLineLoop: LD    HL,(v_currentLine)
        CALL  compareHLDE
        RET   Z
        CALL  nextCurrentLine
        JP    recountLineLoop
drawStatusBar:  PUSH  HL            ; [2]
        LD    HL,(bios_vars.cursorY)
        PUSH  HL
        CALL  enableInverse
        LD    H,90H
        LD    C,30H
statusBarColumn: LD    L,0F1H
        LD    A,09H
statusBarPixel: LD    (HL),0FFH
        INC   L
        DEC   A
        JP    NZ,statusBarPixel
        INC   H
        DEC   C
        JP    NZ,statusBarColumn
        LD    HL,58F9H
        LD    (bios_vars.cursorY),HL
        LD    HL,txtLine
        CALL  bios_printString
        LD    HL,80F9H
        LD    (bios_vars.cursorY),HL
        LD    HL,txtCol
        CALL  bios_printString
        CALL  updateLineNumber
        CALL  updateEditMode
        CALL  disableInverse
        POP   HL
        LD    (bios_vars.cursorY),HL
        POP   HL
        RET

; Снимает собственный адрес возврата, сбрасывает SP сеанса и прыгает по нему.
; Вход: корректный операнд smc_resetSessionStack. Выход: косвенный переход через HL.
; HL разрушен; AF, BC, DE и флаги сохраняются. Это не обычный RET.
resetSessionStack:  POP   HL

        ; Установка указателя стека, агрумент этой инструкции
        ; может меняться из п/п beginSession
smc_resetSessionStack:   LD    SP,8F50H
        ; Переход по адресу в hl
        JP    (HL)

moveNextWord: CALL  beginLineEdit
        CALL  getLineBufferCursor
        LD    DE,lineWordRightLimit             ; 53320
        LD    C,18H         ; 24
skipCurrentWordRight: CALL  compareHLDE
        RET   Z
        LD    A,(HL)
        CP    20H           ; 32 ' '
        JP    Z,skipSpacesRight
        INC   HL
        CALL  bios_printChar
        JP    skipCurrentWordRight
skipSpacesRight: CALL  compareHLDE         ; [2]
        JP    Z,keyHandler_End
        LD    A,(HL)
        CP    20H           ; 32 ' '
        RET   NZ
        INC   HL
        CALL  bios_printChar
        JP    skipSpacesRight
movePreviousWord: CALL  beginLineEdit
        CALL  getLineBufferCursor
        LD    DE,startupBannerAndLineBuffer             ; 53258
        LD    C,08H         ; 8
        CALL  compareHLDE
        RET   Z
        DEC   HL
        DEC   DE
skipSpacesLeft: CALL  compareHLDE
        RET   Z
        LD    A,(HL)
        CP    20H           ; 32 ' '
        JP    NZ,skipCurrentWordLeft
        CALL  bios_printChar
        DEC   HL
        JP    skipSpacesLeft
skipCurrentWordLeft: CALL  compareHLDE         ; [2]
        RET   Z
        LD    A,(HL)
        CP    20H           ; 32 ' '
        RET   Z
        DEC   HL
        CALL  bios_printChar
        JP    skipCurrentWordLeft
; Заливка экранных столбцов с помощью PUSH; строка состояния не затрагивается.
; HL, DE сохранены; AF, C и флаги разрушены. SP восстанавливается из кода.
; Нельзя вызывать с аппаратными прерываниями, использующими этот же стек.
clearTextViewport:  PUSH  HL
        PUSH  DE
        LD    HL,0000H
        ADD   HL,SP
        ; САМОМОДИФИКАЦИЯ: SP до временного использования стека как заливки.
        LD    (smc_restoreClearStack+1),HL
        LD    DE,0000H
        LD    HL,0BFF0H   ; 49136
        LD    C,30H         ; 48 '0'
clearTextColumn: LD    SP,HL
        LD    A,0FH         ; 15
clearTextPushLoop: PUSH  DE
        PUSH  DE
        PUSH  DE
        PUSH  DE
        PUSH  DE
        PUSH  DE
        PUSH  DE
        PUSH  DE
        DEC   A
        JP    NZ,clearTextPushLoop
        DEC   H
        DEC   C
        JP    NZ,clearTextColumn
smc_restoreClearStack:   LD    SP,0FFFFH   ; 65535
        POP   DE
        POP   HL
        RET
enableInverse:  PUSH  HL            ; [5]
        LD    HL,0FFFFH   ; 65535
setVideoInverse: LD    (bios_vars.inverse),HL
        POP   HL
        RET
disableInverse:  PUSH  HL            ; [6]
        LD    HL,0000H
        JP    setVideoInverse

;----------------------------------------------------------------------
; Запрос имени файла в строке состояния

