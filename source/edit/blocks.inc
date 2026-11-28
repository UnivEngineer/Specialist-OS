; Табуляция, страницы и посимвольное выделение
; Порядок включения задан главным .asm; см. README.md.

keyHandler_Tab:
        LD C,9
        JP keyHandler_Space

editorPageUp:
        CALL editorSaveVerticalGoal
        LD HL,editorFinishVerticalMove
        PUSH HL
        LD HL,(v_currentLine)
        EX DE,HL
        LD HL,(v_topLine)
        CALL compareHLDE
        JP Z,editorPageUpAtTop
        LD    HL,(v_textBeginLo)
        EX    DE,HL
        LD    HL,(v_topLine)
        LD    B,EDITOR_PAGE_STEP
pageUpLineLoop:  CALL  compareHLDE
        JP    Z,pageUpRedraw
        CALL  previousLine
        PUSH  HL
        CALL  previousCurrentLine
        POP   HL
        DEC   B
        JP    NZ,pageUpLineLoop
pageUpRedraw:  LD    (v_topLine),HL
        LD A,B
        OR A
        LD A,2
        JP Z,editorPageRetainAndDraw
        CALL  redrawPreservingCursor
        RET

editorPageDown:
        CALL editorSaveVerticalGoal
        LD HL,editorFinishVerticalMove
        PUSH HL
        LD HL,(v_currentLine)
        EX DE,HL
        LD HL,(v_topLine)
        CALL compareHLDE
        JP Z,editorPageDownAtTop
        LD    HL,(v_topLine)
        LD    BC,EDITOR_PAGE_STEP*256+19h
pageDownLineLoop:  LD    E,L
        LD    D,H
        CALL  nextLine
        CALL gapTextRead
        OR    A
        JP    NZ,pageDownAdvance
        LD    L,E
        LD    H,D
        JP    pageDownRedraw
pageDownAdvance:  PUSH  HL
        CALL  nextCurrentLine
        CALL  Z,mon2_printCharOld
        POP   HL
        DEC   B
        JP    NZ,pageDownLineLoop
pageDownRedraw:  LD    (v_topLine),HL
        LD A,B
        OR A
        LD A,1
        JP Z,editorPageRetainAndDraw
        CALL  redrawPreservingCursor
        RET

; Common case: cursor on the first visible row. Walk the document once,
; rather than scanning each line separately for top and current pointers.
editorPageDownAtTop:
        LD B,EDITOR_PAGE_STEP
        LD C,0
editorPageDownFast:
        CALL gapTextRead
        OR A
        JP Z,editorPageDownFastDone
        CALL nextLine
        INC C
        DEC B
        JP NZ,editorPageDownFast
editorPageDownFastDone:
        CALL editorPageSetTop
        LD A,C
        CP EDITOR_PAGE_STEP
        JP NZ,redrawPreservingCursor
        LD A,1
        JP editorPageRetainAndDraw
editorPageUpAtTop:
        EX DE,HL
        LD HL,(v_textBeginLo)
        EX DE,HL
        LD B,EDITOR_PAGE_STEP
        LD C,0
editorPageUpFast:
        CALL compareHLDE
        JP Z,editorPageUpFastDone
        CALL previousLine
        DEC C
        DEC B
        JP NZ,editorPageUpFast
editorPageUpFastDone:
        CALL editorPageSetTop
        LD A,C
        CP -EDITOR_PAGE_STEP & 255
        JP NZ,redrawPreservingCursor
        LD A,2
        JP editorPageRetainAndDraw
editorPageSetTop:
        LD (v_topLine),HL
        LD (v_currentLine),HL
        LD E,C
        LD D,0
        LD A,C
        OR A
        JP P,editorPageAddLines
        DEC D
editorPageAddLines:
        LD HL,(v_lineNumber)
        ADD HL,DE
        LD (v_lineNumber),HL
        RET

; A=1 down / 2 up. Reuse the two overlap rows through the public SDK,
; including their cached visible lengths; repaint only 21 new rows.
editorPageRetainAndDraw:
        LD (editorPageRetained),A
        CP 1
        JP NZ,editorPageCopyUp
        LD HL,editorPageWidths+EDITOR_PAGE_STEP*3
        LD DE,editorPageWidths+EDITOR_ROWS*3
        LD BC,editorPageWidths
        CALL mon2_memcpy_bc_hl
        LD HL,(editorRowLengths+EDITOR_PAGE_STEP)
        LD (editorRowLengths),HL
        LD HL,09000h+EDITOR_TEXT_TOP+EDITOR_PAGE_STEP*10
        LD BC,09000h+EDITOR_TEXT_TOP
        JP editorPageCopy
editorPageCopyUp:
        LD HL,editorPageWidths
        LD DE,editorPageWidths+6
        LD BC,editorPageWidths+EDITOR_PAGE_STEP*3
        CALL mon2_memcpy_bc_hl
        LD HL,(editorRowLengths)
        LD (editorRowLengths+EDITOR_PAGE_STEP),HL
        LD HL,09000h+EDITOR_TEXT_TOP
        LD BC,09000h+EDITOR_TEXT_TOP+EDITOR_PAGE_STEP*10
editorPageCopy:
        COLOR COLOR_TEXT
        LD A,(editorCopyWidth)
        LD D,A
        LD E,20
        CALL uiCopyTextRect
        JP redrawPreservingCursor

keyHandler_DocumentEnd:
        CALL editorResetGoal
editorDocumentEndNext:
        CALL nextCurrentLine
        JP NZ,editorDocumentEndNext
        CALL editorLineLength
        LD (v_column),HL
        LD HL,(v_currentLine)
        LD B,0
editorDocumentEndTop:
        LD A,B
        CP EDITOR_ROWS-1
        JP Z,editorDocumentEndDraw
        EX DE,HL
        LD HL,(v_textBeginLo)
        EX DE,HL
        CALL compareHLDE
        JP Z,editorDocumentEndDraw
        CALL previousLine
        INC B
        JP editorDocumentEndTop
editorDocumentEndDraw:
        LD (v_topLine),HL
        LD A,EDITOR_FIRST_Y
editorDocumentEndY:
        DEC B
        JP M,editorDocumentEndReady
        ADD A,10
        JP editorDocumentEndY
editorDocumentEndReady:
        LD (mon2_vars.cursorY),A
        JP redrawPreservingCursor

editorMarkActive: DB 0
editorMarkAnchor: DW 0
editorFreezeMark:
        LD A,(editorMarkActive)
        OR A
        RET Z
        XOR A
        LD (editorMarkActive),A
        PUSH BC
        CALL editorToolbar
        POP BC
        RET
editorToggleMark:
        LD A,(editorMarkActive)
        OR A
        JP Z,editorMarkBeginOrClear
        XOR A
        LD (editorMarkActive),A
        JP editorToolbar
editorMarkBeginOrClear:
        LD HL,(v_selectionBegin)
        LD A,H
        OR L
        JP NZ,editorClearSelection
        CALL editorMarkPointer
        LD (editorMarkAnchor),HL
        LD A,1
        LD (editorMarkActive),A
        CALL editorUpdateMark
        JP editorToolbar
editorUpdateMark:
        LD A,(editorMarkActive)
        OR A
        RET Z
        CALL editorMarkPointer
        EX DE,HL
        LD HL,(editorMarkAnchor)
        CALL compareHLDE
        JP NC,editorMarkOrdered
        EX DE,HL
editorMarkOrdered:
        CALL compareHLDE
        JP NZ,editorMarkNonempty
        LD HL,0
        LD D,H
        LD E,L
editorMarkNonempty:
        PUSH HL
        LD HL,(v_selectionBegin)
        CALL compareHLDE
        JP NZ,editorMarkChanged
        POP HL
        PUSH HL
        PUSH DE
        EX DE,HL
        LD HL,(v_selectionEnd)
        CALL compareHLDE
        POP DE
        JP Z,editorMarkUnchanged
editorMarkChanged:
        EX DE,HL
        LD (v_selectionBegin),HL
        POP HL
        LD (v_selectionEnd),HL
        JP editorRedrawMark
editorMarkUnchanged:
        POP HL
        RET

editorDeleteSelection:
        LD HL,(v_selectionBegin)
        LD A,H
        OR L
        JP Z,Beep4
        LD (editorBlockResult),HL
        CALL commitLineEdit
        ; Make the selected bytes contiguous just to the right of the gap.
        CALL gapMove
        CALL editorRemoveCountedSelection
        JP editorBlockReposition
; Remove the current byte interval. The caller saved its desired caret.
editorRemoveCountedSelection:
        LD HL,(v_selectionEnd)
        EX DE,HL
        LD HL,(v_selectionBegin)
        CALL editorRangeBreaks
        PUSH BC
        CALL editorSelectionRemove
        POP BC
        JP editorSubtractLines
editorSelectionRemove:
        LD HL,(v_selectionBegin)
        EX DE,HL
        LD HL,(v_selectionEnd)
        PUSH HL
        LD HL,0
        LD (v_selectionBegin),HL
        LD (v_selectionEnd),HL
        XOR A
        LD (editorMarkActive),A
        POP HL
        JP resizeTextRange

editorClearSelection:
        XOR A
        LD (editorMarkActive),A
        LD HL,0
        LD (v_selectionBegin),HL
        LD (v_selectionEnd),HL
        CALL redrawSelection
        JP editorToolbar
redrawSelection:
        LD A,(hexActive)
        OR A
        JP NZ,hexRefresh
        CALL  saveCursor
        LD    HL,(v_topLine)
        CALL  drawPage
        JP    restoreCursor
documentOutOfMemory:
        LD HL,editorMemoryMessage
        CALL editorErrorModal
        CALL editorCloseModal
        CALL resetSessionStack
        JP editorStatusLoop

