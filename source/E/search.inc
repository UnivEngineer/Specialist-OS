; Поиск, разбиение и объединение строк, копирование блоков
; Порядок включения задан главным .asm; см. README.md.

; Поиск начинается ПОСЛЕ верхней строки страницы (v_topLine), не строки курсора.
; Enter повторяет прежний образец; новый ввод ограничен 14 символами.
; Разрушает AF, BC, DE, HL, флаги; совпадение становится верхней строкой страницы.
keyHandler_F7:
        CALL  commitLineEdit
        LD    HL,06F9H    ; 1785
        LD    (bios_vars.cursorY),HL
        CALL  enableInverse
        LD    HL,txtSearchPrompt    ; 53395
        CALL  bios_printString
        LD    HL,1EF9H    ; 7929
        LD    (bios_vars.cursorY),HL
        CALL  bios_getch
        CP    0DH           ; 13
        JP    Z,repeatSearch
        LD    BC,1020H    ; 4128
        CALL  TypeCharCtimesB
        LD    (bios_vars.cursorY),HL
        LD    HL,v_searchText    ; 53403
        LD    E,L
        LD    D,H
        LD    B,0FH         ; 15
        CALL  promptTextInput
        PUSH  AF
        CALL  disableInverse
        POP   AF
        JP    NZ,synchronizeTopAndRedraw
repeatSearch:  CALL  disableInverse
        LD    HL,(v_topLine)
        CALL  nextLine
searchCandidate:  LD    DE,v_searchText    ; 53403
compareSearchByte:  LD    B,(HL)
        LD    A,(DE)
        OR    A
        JP    Z,searchFound
        CP    B
        INC   HL
        INC   DE
        JP    Z,compareSearchByte
        INC   B
        JP    NZ,searchCandidate
        CALL  Beep4
        JP    keyHandler_Esc_Home
searchFound:  CALL  previousLine
        LD    (v_topLine),HL
        LD    (v_currentLine),HL
        CALL  recountCurrentLine
        JP    synchronizeTopAndRedraw

keyHandler_Esc_J:
        CALL  beginLineEdit
        CALL  trimLineBuffer
        LD    HL,(v_currentLine)
        CALL  nextLine
        LD    A,(HL)
        INC   A
        RET   Z
        DEC   HL
        LD    (HL),20H    ; 32 ' '
        INC   HL
        LD    C,L
        LD    B,H
        LD    HL,lineJoinLimit             ; 53319
joinLineByte:  CALL  compareHLDE
        JP    C,commitAndRedraw
        LD    A,(BC)
        CP    0DH           ; 13
        JP    Z,commitAndRedraw
        CP    0FFH          ; 255
        JP    Z,commitAndRedraw
        LD    (DE),A
        INC   DE
        INC   BC
        JP    joinLineByte

keyHandler_Esc_S:
        LD    HL,(v_currentLine)
        CALL  addCursorColumn
        LD    E,L
        LD    D,H
        INC   DE
        PUSH  HL
        CALL  resizeTextRange
        POP   HL
        LD    (HL),0DH    ; 13
        JP    redrawPreservingCursor

; Вход: HL = указатель в тексте. Выход: HL = начало предыдущей строки.
; Z = достигнуто начало текста, NZ = найден переход; DE сохранён, AF разрушен.
previousLine:  PUSH  DE            ; [6]
        EX    DE,HL
        LD    HL,(v_textBeginLo)
        EX    DE,HL
        CALL  compareHLDE
        JP    Z,previousLineReturn
        DEC   HL
        CALL  compareHLDE
        JP    Z,previousLineFound
        DEC   HL
findPreviousLineLoop:  LD    A,(HL)
        CP    0DH           ; 13
        JP    Z,previousLineAfterCR
        CALL  compareHLDE
        DEC   HL
        JP    NZ,findPreviousLineLoop

previousLineAfterCR:  INC   HL
previousLineFound: XOR   A
        INC   A

previousLineReturn: POP   DE
        RET

keyHandler_Esc_C:
        LD    HL,(v_selectionEnd)
        EX    DE,HL
        LD    HL,(v_selectionBegin)
        LD    A,H
        OR    L
        JP    Z,Beep4
        CALL  subtractHLFromDE
        LD    HL,(v_currentLine)
        CALL  testLineSelected
        PUSH  AF
        CALL  nextLine
        POP   AF
        CALL  C,testLineSelected
        JP    C,Beep4
        PUSH  HL
        EX    DE,HL
        ADD   HL,DE
        EX    DE,HL
        CALL  resizeTextRange
        LD    HL,(v_selectionBegin)
        LD    C,L
        LD    B,H
        POP   HL
        CALL  copyUntilDE
        JP    redrawPreservingCursor

keyHandler_Esc_M:
        LD    HL,(v_selectionEnd)
        EX    DE,HL
        LD    HL,(v_selectionBegin)
        LD    A,H
        OR    L
        JP    Z,Beep4
        CALL  subtractHLFromDE
        LD    HL,(v_currentLine)
        CALL  testLineSelected
        JP    C,Beep4
        CALL  nextLine
        PUSH  HL
        EX    DE,HL
        ADD   HL,DE
        EX    DE,HL
        POP   BC
        PUSH  DE
        PUSH  HL
        PUSH  BC
        CALL  resizeTextRange
        LD    HL,(v_selectionBegin)
        LD    C,L
        LD    B,H
        POP   HL
        CALL  copyUntilDE
        LD    HL,(v_selectionBegin)
        EX    DE,HL
        POP   HL
        LD    (v_selectionBegin),HL
        LD    HL,(v_selectionEnd)
        EX    (SP),HL
        LD    (v_selectionEnd),HL
        POP   HL
        JP    deleteRangeAndReposition

keyHandler_Esc_N:
        LD    HL,txtNew    ; 53419
        CALL  bios_printString
        CALL  bios_getch
        CP    59H           ; 89 'Y'
        JP    NZ,synchronizeTopAndRedraw
        LD    HL,(v_textBeginLo)
        LD    (HL),0DH    ; 13
        INC   HL
        LD    (HL),0FFH   ; 255
        LD    (v_textEnd),HL
        JP    keyHandler_Esc_Home

keyHandler_F8:
        XOR   A
        LD    (v_lineDirty),A
        CALL  saveCursor
        CALL  keyHandler_Home
        LD    BC,3F20H    ; 16160
        CALL  TypeCharCtimesB
        LD    HL,(v_currentLine)
        CALL  drawTextLine
        JP    restoreCursor
