; Редактирование строки и изменение размера текста
; Порядок включения задан главным .asm; см. README.md.

keyHandler_CTP:
        COLOR COLOR_BIOS
        LD    C,1FH
        CALL  bios_printChar
        CALL  commitLineEdit
        CALL  resetSessionStack
        JP    bios_reboot

keyHandler_F3_2:
        CALL  commitLineEdit
        COLOR COLOR_BIOS
        LD    C,1FH
        CALL  bios_printChar
        CALL  resetSessionStack
        RET

keyHandler_Esc_Home:
        LD    HL,0001H    ; 1; [4]
        LD    (v_lineNumber),HL
        LD    HL,(v_textBeginLo)
        LD    (v_topLine),HL
        LD    (v_currentLine),HL
synchronizeTopAndRedraw:   LD    HL,(v_topLine)   ; [10]
        EX    DE,HL
        LD    HL,(v_currentLine)
        CALL  compareHLDE
        JP    Z,clearAndDrawPage
        CALL  previousCurrentLine
        JP    synchronizeTopAndRedraw
clearAndDrawPage:   CALL  clearTextViewport         ; [2]
drawPage:   LD    BC,180CH    ; 6156; [2]
drawPageLine:  CALL  bios_printChar
        CALL  drawTextLine
        INC   A
        JP    Z,finishPageAndHome
        DEC   B
        LD    C,0AH         ; 10
        JP    NZ,drawPageLine
finishPageAndHome:   CALL  drawStatusBar         ; [2]
        LD    C,0CH         ; 12
        JP    bios_printChar
commitAndRedraw:  CALL  commitLineEdit         ; [3]
redrawPreservingCursor:   CALL  saveCursor         ; [5]
        LD    HL,(v_topLine)
        CALL  clearAndDrawPage
        JP    restoreCursor

keyHandler_Down:
        CALL  commitLineEdit
        CALL  nextCurrentLine
        RET   Z
        CALL  updateLineNumber
        LD    A,(bios_vars.cursorY)
        CP    0EEH          ; 238
        JP    NZ,bios_printChar
        LD    HL,(v_topLine)
        CALL  nextLine
        LD    (v_topLine),HL
        CALL  saveCursor
        CALL  scrollTextUp
        LD    HL,(v_currentLine)
        CALL  drawTextLine
        JP    restoreCursor

keyHandler_Up
        CALL  commitLineEdit
        CALL  previousCurrentLine
        RET   Z
        CALL  updateLineNumber
        LD    A,(bios_vars.cursorY)
        CP    08H           ; 8
        JP    NZ,bios_printChar
        LD    HL,(v_currentLine)
        LD    (v_topLine),HL
        PUSH  HL
        CALL  insertScreenRow
        POP   HL
        CALL  saveCursor
        CALL  drawTextLine
        JP    restoreCursor
drawTextLine:   CALL  keyHandler_Home          ; [4]
        CALL  testLineSelected
        JP    NC,drawLineByte
        LD    A,0FFH                ; 255
        LD    (bios_vars.inverse),A
drawLineByte:  LD    A,(HL)
        CP    0DH           ; 13
        LD    C,20H         ; 32 ' '
        CALL  Z,bios_printChar
drawLineLoop:  LD    A,(HL)
        LD    C,A
        INC   A
        JP    Z,finishDrawLine
        INC   HL
        CALL  bios_printChar
        CP    0EH           ; 14
        JP    NZ,drawLineLoop
finishDrawLine:  XOR   A
        LD    (bios_vars.inverse),A
        LD    (bios_vars.inverse+1),A
        LD    A,C
        RET
testLineSelected: PUSH  DE            ; [7]
        EX    DE,HL
        LD    HL,(v_selectionBegin)
        LD    A,H
        OR    L
        JP    Z,selectionTestDone
        DEC   HL
        CALL  compareHLDE
        JP    NC,selectionTestDone
        LD    HL,(v_selectionEnd)
        DEC   HL
        CALL  compareHLDE
        CCF
selectionTestDone:  EX    DE,HL         ; [2]
        POP   DE
        RET

keyHandler_Right:
        CALL  testShiftKey
        JP    Z,moveNextWord
moveRightWithinLine:  LD    B,0BAH                ; 186
        JP    moveHorizontalWithinLine

keyHandler_Left:
        CALL  testShiftKey
        JP    Z,movePreviousWord
        LD    B,00H
moveHorizontalWithinLine:  LD    A,(bios_vars.cursorY+1)
        CP    B
        RET   Z
        JP    bios_printChar

keyHandler_End:
        CALL  beginLineEdit
        CALL  trimLineBuffer
        LD    HL,startupBannerAndLineBuffer             ; 53258
        CALL  subtractHLFromDE
        LD    A,E
        JP    clampCursorColumn

keyHandler_Home:
        XOR   A             ; [4]
        LD    (bios_vars.cursorY+1),A
        RET

keyHandler_Enter:
        LD    HL,(v_currentLine)
        CALL  testLineSelected
        JP    C,Beep4
        CALL  beginLineEdit
        CALL  trimLineBuffer
        LD    A,0DH         ; 13
        LD    (DE),A
        CALL  commitLineEdit
        CALL  keyHandler_Home
        LD    HL,(v_currentLine)
        LD    C,18H         ; 24
indentInsertedLine:  LD    A,(HL)
        CP    20H           ; 32 ' '
        JP    NZ,moveToInsertedLine
        CALL  bios_printChar
        INC   HL
        JP    indentInsertedLine
moveToInsertedLine:  LD    C,1AH         ; 26
        CALL  keyHandler_Down
        JP    insertScreenRow

        ; В lineBufferEnd изначально текст "KSOFT '92"
trimLineBuffer:  LD    DE, lineBufferEnd

        ; Пропус пробелов в строке по адресу de
SkipSpaces:
        DEC   DE
        LD    A,(DE)
        CP    20H
        JP    Z,SkipSpaces
        INC   DE
        RET

; Вход: v_currentLine. Выход: v_lineDirty != 0, буфер строки заполнен.
; Разрушает AF, B, DE, HL, флаги. Перезаписывает 64 байта стартового КОДА!
beginLineEdit:  LD    A,(v_lineDirty)    ; [7]
        OR    A
        RET   NZ
        cpl
        LD    (v_lineDirty),A
        LD    HL,startupBannerAndLineBuffer             ; 53258
        LD    A,40H         ; 64 '@'
clearLineBufferLoop:  LD    (HL),20H    ; 32 ' '
        INC   HL
        DEC   A
        JP    NZ,clearLineBufferLoop
        LD    DE,startupBannerAndLineBuffer             ; 53258
        LD    HL,(v_currentLine)
        LD    B,40H         ; 64 '@'
copyLineToBufferLoop:  LD    A,(HL)
        CP    0DH           ; 13
        RET   Z
        CP    0FFH          ; 255
        RET   Z
        DEC   B
        RET   Z
        LD    (DE),A
        INC   HL
        INC   DE
        JP    copyLineToBufferLoop

keyHandler_Space:
        LD    HL,(v_currentLine)
        CALL  testLineSelected
        JP    C,Beep4
        CALL  beginLineEdit
        CALL  getLineBufferCursor
        LD    A,(v_insertMode)
        OR    A
        JP    NZ,insertTypedCharacter
        LD    (HL),C
        CALL  bios_printChar
        LD    A,(bios_vars.cursorY+1)
        CP    0BDH          ; 189
        RET   NZ
        LD    C,08H         ; 8
        JP    bios_printChar
insertTypedCharacter:  LD    DE,lineBufferLast             ; 53321
        CALL  SkipSpaces
        PUSH  HL
        LD    HL,lineBufferLast             ; 53321
        CALL  compareHLDE
        JP    Z,drawInsertedTail
        INC   DE
drawInsertedTail:  POP   HL
        CALL  saveCursor
insertCharacterLoop:  LD    B,(HL)
        LD    (HL),C
        CALL  bios_printChar
        LD    C,B
        INC   HL
        CALL  compareHLDE
        JP    C,insertCharacterLoop
        CALL  restoreCursor
        LD    C,18H         ; 24
        JP    moveRightWithinLine

keyHandler_F1:
        LD    HL,v_insertMode             ; 53340
        LD    A,(HL)
        cpl
        LD    (HL),A
        JP    updateEditMode
getLineBufferCursor:  LD    HL,startupBannerAndLineBuffer             ; 53258; [4]
addCursorColumn:  LD    A,(bios_vars.cursorY+1)        ; [4]
        OR    A
cursorColumnLoop:  RET   Z
        INC   HL
        SUB   03H           ; 3
        JP    cursorColumnLoop

keyHandler_Del:
        CALL  beginLineEdit
        CALL  getLineBufferCursor
        PUSH  HL
        LD    C,L
        LD    B,H
        INC   BC
        LD    DE,lineBufferLast             ; 53321
        CALL  copyUntilDE
        POP   HL
        LD    DE,lineBufferLast             ; 53321
        CALL  SkipSpaces
        PUSH  HL
        LD    HL,lineBufferLast             ; 53321
        CALL  compareHLDE
        JP    Z,drawDeletedTail
        INC   DE
drawDeletedTail:  POP   HL
        CALL  saveCursor
redrawTailLoop:  LD    C,(HL)
        CALL  bios_printChar
        CALL  compareHLDE
        INC   HL
        JP    C,redrawTailLoop
        JP    restoreCursor

; Вход: изменённая строка и v_currentLine. Выход: строка возвращена в текст.
; BC сохранён; AF, DE, HL, флаги разрушены. При нехватке памяти — перезапуск просмотра.
commitLineEdit:  LD    A,(v_lineDirty)  ; читаем v_lineDirty
        OR    A
        RET   Z         ; если v_lineDirty == 0, выходим
        XOR   A         ; инвертируем
        LD    (v_lineDirty),A  ; сохраняем обртано
        PUSH  BC
        CALL  trimLineBuffer
        LD    HL,startupBannerAndLineBuffer
        CALL  subtractHLFromDE
        LD    HL,(v_currentLine)
        EX    DE,HL
        ADD   HL,DE
        EX    DE,HL
        CALL  nextLine
        INC   A
        JP    Z,resizeEditedLine
        DEC   HL
resizeEditedLine:  CALL  resizeTextRange
        LD    HL,(v_currentLine)
        LD    BC,startupBannerAndLineBuffer             ; 53258
        CALL  compareHLDE
        CALL  NZ,copyUntilDE
        POP   BC
        RET
; Вход: HL = прежняя граница, DE = новая граница хвоста документа.
; Выход: хвост сдвинут, v_textEnd и выделение обновлены.
; Разрушает AF, BC, HL, флаги; DE сохраняется на обычном пути.
; При расширении проверяет v_textLimitLo, при ошибке уходит в documentOutOfMemory.
resizeTextRange:  CALL  compareHLDE         ; [5]
        RET   Z
        PUSH  AF
        PUSH  DE
        CALL  subtractHLFromDE
        LD    B,D
        LD    C,E
        EX    DE,HL
        LD    HL,(v_selectionBegin)
        CALL  compareHLDE
        JP    C,adjustSelectionEnd
        ADD   HL,BC
        LD    (v_selectionBegin),HL
adjustSelectionEnd:  LD    HL,(v_selectionEnd)
        EX    DE,HL
        CALL  compareHLDE
        EX    DE,HL
        JP    NC,moveDocumentTail
        ADD   HL,BC
        LD    (v_selectionEnd),HL
moveDocumentTail:  EX    DE,HL
        POP   DE
        POP   AF
        JP    C,expandDocumentTail
        PUSH  DE
        LD    C,E
        LD    B,D
        EX    DE,HL
        LD    HL,(v_textEnd)
        INC   HL
        EX    DE,HL
        CALL  subtractHLFromDE
compactTailPairs:  LD    A,E
        AND   0FEH          ; 254
        OR    D
        JP    Z,compactTailRemainder
        LD    A,(HL)
        LD    (BC),A
        INC   BC
        INC   HL
        DEC   DE
        LD    A,(HL)
        LD    (BC),A
        INC   BC
        INC   HL
        DEC   DE
        JP    compactTailPairs
compactTailRemainder:  CP    E
        JP    Z,rememberCompactedEnd
        LD    A,(HL)
        LD    (BC),A
        INC   BC
rememberCompactedEnd:  DEC   BC
        LD    L,C
        LD    H,B
        LD    (v_textEnd),HL
        POP   DE
        RET
expandDocumentTail:  PUSH  DE
        CALL  subtractHLFromDE
        LD    HL,(v_textEnd)
        LD    B,H
        LD    C,L
        ADD   HL,DE
        POP   DE
        PUSH  DE
        PUSH  HL
        EX    DE,HL
        LD    HL,(v_textLimitLo)
        CALL  compareHLDE
        JP    C,documentOutOfMemory
        POP   HL
        POP   DE
        LD    (v_textEnd),HL
        PUSH  HL
        EX    DE,HL
        CALL  subtractHLFromDE
        EX    (SP),HL
        INC   DE
expandTailQuads:  LD    A,E
        AND   0FCH          ; 252
        OR    D
        JP    Z,expandTailRemainder
        LD    A,(BC)
        LD    (HL),A
        DEC   HL
        DEC   BC
        DEC   DE
        LD    A,(BC)
        LD    (HL),A
        DEC   HL
        DEC   BC
        DEC   DE
        LD    A,(BC)
        LD    (HL),A
        DEC   HL
        DEC   BC
        DEC   DE
        LD    A,(BC)
        LD    (HL),A
        DEC   HL
        DEC   BC
        DEC   DE
        JP    expandTailQuads
expandTailRemainder:  LD    A,E
        OR    A
        JP    Z,finishExpandTail
expandTailByte:  LD    A,(BC)
        LD    (HL),A
        DEC   HL
        DEC   BC
        DEC   E
        JP    NZ,expandTailByte
finishExpandTail:  POP   DE
        RET

