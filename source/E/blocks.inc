; Табуляция, страницы и выделение строк
; Порядок включения задан главным .asm; см. README.md.

keyHandler_Tab:
        CALL  testShiftKey
        JP    Z,tabPreviousStop
        LD    HL,0001H    ; 1
        CALL  addCursorColumn
        LD    E,L
        LD    HL,t_tabSteps             ; 53347
        LD    C,03H         ; 3
        XOR   A
tabNextInitialStop:  DEC   C
        JP    Z,tabNextRepeatedStop
        ADD   A,(HL)
        CP    E
        INC   HL
        JP    C,tabNextInitialStop
clampCursorColumn:  CP    3EH           ; 62 '>'; [2]
        JP    C,setCursorColumn
        LD    A,3EH         ; 62 '>'
setCursorColumn:  LD    L,A           ; [3]
        ADD   A,A
        ADD   A,L
        LD    (bios_vars.cursorY+1),A
        RET
tabNextRepeatedStop:  ADD   A,(HL)                ; [2]
        CP    E
        JP    C,tabNextRepeatedStop
        JP    clampCursorColumn
tabPreviousStop:  LD    HL,0000H
        CALL  addCursorColumn
        LD    E,L
        LD    HL,t_tabSteps             ; 53347
        LD    C,03H         ; 3
        XOR   A
tabPreviousInitialStop:  DEC   C
        JP    Z,tabPreviousRepeatedStop
        ADD   A,(HL)
        CP    E
        INC   HL
        JP    C,tabPreviousInitialStop
        DEC   HL
        SUB   (HL)
        JP    setCursorColumn
tabPreviousRepeatedStop:  ADD   A,(HL)                ; [2]
        CP    E
        JP    C,tabPreviousRepeatedStop
        SUB   (HL)
        JP    setCursorColumn

keyHandler_F2:
        CALL  commitLineEdit         ; [2]
        LD    HL,(v_textBeginLo)
        EX    DE,HL
        LD    HL,(v_topLine)
        LD    B,16H         ; 22
pageUpLineLoop:  CALL  compareHLDE
        JP    Z,pageUpRedraw
        CALL  previousLine
        PUSH  HL
        CALL  previousCurrentLine
        POP   HL
        DEC   B
        JP    NZ,pageUpLineLoop
pageUpRedraw:  LD    (v_topLine),HL
        CALL  redrawPreservingCursor
        RET

keyHandler_F3:
        CALL  commitLineEdit
        LD    HL,(v_topLine)
        LD    BC,1619H    ; 5657
pageDownLineLoop:  LD    E,L
        LD    D,H
        CALL  nextLine
        LD    A,(HL)
        INC   A
        JP    NZ,pageDownAdvance
        LD    L,E
        LD    H,D
        JP    pageDownRedraw
pageDownAdvance:  PUSH  HL
        CALL  nextCurrentLine
        CALL  Z,bios_printChar
        POP   HL
        DEC   B
        JP    NZ,pageDownLineLoop
pageDownRedraw:  LD    (v_topLine),HL
        CALL  redrawPreservingCursor
        RET

keyHandler_Esc_End:
        CALL  nextCurrentLine
        JP    NZ,keyHandler_Esc_End
        LD    HL,(v_currentLine)
        LD    (v_topLine),HL
        LD    A,08H         ; 8
        LD    (bios_vars.cursorY),A
        JP    keyHandler_F2

keyHandler_Esc_L:
        LD    HL,redrawSelection    ; 54885
        PUSH  HL
        LD    HL,(v_selectionBegin)
        LD    A,H
        OR    L
        JP    NZ,extendSelection
        CALL  setSelectionBegin
        JP    setSelectionEnd

extendSelection:  EX    DE,HL
        LD    HL,(v_currentLine)
        CALL  nextLine
        EX    DE,HL
        CALL  compareHLDE
        JP    C,setSelectionEnd
setSelectionBegin:  LD    HL,(v_currentLine)
        LD    (v_selectionBegin),HL
        RET

setSelectionEnd:  LD    HL,(v_currentLine)   ; [2]
        CALL  nextLine
        LD    (v_selectionEnd),HL
        RET

keyHandler_Esc_D:
        LD    HL,(v_selectionBegin)
        LD    A,L
        OR    H
        JP    Z,Beep4
        EX    DE,HL
        LD    HL,(v_currentLine)
findLineBeforeSelection:  CALL  testLineSelected
        JP    NC,deleteSelectionRange
        CALL  previousLine
        JP    NZ,findLineBeforeSelection
deleteSelectionRange:  LD    (v_currentLine),HL
        LD    HL,0000H
        LD    (v_selectionBegin),HL
        LD    HL,(v_selectionEnd)
deleteRangeAndReposition:  PUSH  DE
        CALL  subtractHLFromDE
        LD    B,D
        LD    C,E
        EX    DE,HL
        LD    HL,(v_topLine)
        CALL  compareHLDE
        JP    C,adjustCurrentAfterDelete
        ADD   HL,BC
        LD    (v_topLine),HL
adjustCurrentAfterDelete:  LD    HL,(v_currentLine)
        CALL  compareHLDE
        JP    C,collapseDeletedRange
        ADD   HL,BC
        LD    (v_currentLine),HL
collapseDeletedRange:  EX    DE,HL
        POP   DE
        CALL  resizeTextRange
        CALL  recountCurrentLine
        LD    A,(bios_vars.cursorY)
        SUB   08H           ; 8
        LD    B,A
        JP    Z,redrawAfterDelete
findTopAfterDelete:  CALL  previousLine
        CALL  Z,moveUpIfAtDocumentEnd
        LD    A,B
        SUB   0AH           ; 10
        LD    B,A
        JP    NZ,findTopAfterDelete
redrawAfterDelete:  LD    (v_topLine),HL
        JP    redrawPreservingCursor
moveUpIfAtDocumentEnd:  PUSH  HL
        CALL  nextCurrentLine
        POP   HL
        RET   NZ
        LD    C,19H         ; 25
        JP    bios_printChar

keyHandler_Esc_U:
        LD    HL,(v_selectionBegin)
        LD    DE,0000H
        CALL  compareHLDE
        RET   Z
        EX    DE,HL
        LD    (v_selectionBegin),HL
redrawSelection:  CALL  saveCursor
        LD    HL,(v_topLine)
        CALL  drawPage
        JP    restoreCursor
documentOutOfMemory:  LD    HL,txtOutOfMemory             ; 53351; [2]
        CALL  bios_printString
        CALL  Beep4
        CALL  bios_getch
        JP    scanDocument

