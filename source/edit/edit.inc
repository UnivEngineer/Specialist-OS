; Правки документа через зазор; снимок строки F8 занимает часть зазора.
keyHandler_CTP:
        LD HL,editorExitNow
        JP editorConfirmAction
editorConfirmAction:
        LD (editorPendingAction),HL
        LD A,(v_documentDirty)
        OR A
        JP NZ,editorExitPrompt
editorRunPendingAction:
        LD HL,(editorPendingAction)
        JP (HL)
editorPendingAction: DW editorExitNow
editorExitNow:
    IF EDIT_BANKED
        LD A,(ebQuiet)
        OR A
        JP NZ,ebReturn
    ENDIF
        COLOR COLOR_BIOS
        LD C,31
        CALL bios_printChar
        CALL resetSessionStack
        JP bios_reboot
keyHandler_DocumentHome:
        CALL editorResetGoal
        LD A,EDITOR_FIRST_Y
        LD (bios_vars.cursorY),A
        LD HL,0
        LD (v_leftColumn),HL
        XOR A
        LD (v_leftColumnHi),A
        LD HL,1
        LD (v_lineNumber),HL
        LD HL,(v_textBeginLo)
        LD (v_topLine),HL
        LD (v_currentLine),HL
        CALL keyHandler_Home
synchronizeTopAndRedraw:
        CALL editorToolbar
        CALL drawStatusBar
synchronizeTopLoop:
        LD HL,(v_topLine)
        EX DE,HL
        LD HL,(v_currentLine)
        CALL compareHLDE
        JP Z,clearAndDrawPage
        CALL previousCurrentLine
        JP synchronizeTopLoop
; Paint only text and its old tail, rounded to complete four-glyph packets.
clearAndDrawPage:
drawPage:
        LD A,(editorPageRetained)
        LD C,A
        XOR A
        LD (editorPageRetained),A
        LD A,EDITOR_FIRST_Y
        LD B,EDITOR_ROWS
        DEC C
        JP Z,editorPageDownRows
        DEC C
        JP NZ,editorPageRowsReady
        LD B,EDITOR_PAGE_STEP
        JP editorPageRowsReady
editorPageDownRows:
        CALL nextLine
        CALL nextLine
        LD A,EDITOR_FIRST_Y+20
        LD B,EDITOR_PAGE_STEP
editorPageRowsReady:
        CALL editorPaintRows
        JP finishPageAndHome
editorPaintRows:
        LD (bios_vars.cursorY),A
drawPageLine:
        CALL drawTextLine
        LD A,(bios_vars.cursorY)
        ADD A,10
        LD (bios_vars.cursorY),A
        DEC B
        JP NZ,drawPageLine
        RET
finishPageAndHome:
        LD A,EDITOR_FIRST_Y
        LD (bios_vars.cursorY),A
        RET
redrawPreservingCursor:
        CALL saveCursor
        COLOR COLOR_TEXT
        LD HL,(v_topLine)
        CALL drawPage
        JP restoreCursor
keyHandler_Down:
        CALL editorSaveVerticalGoal
        LD HL,editorFinishVerticalMove
        PUSH HL
        CALL nextCurrentLine
        RET Z
        LD A,(bios_vars.cursorY)
        CP EDITOR_LAST_Y
        JP Z,editorScrollDown
        ADD A,10
        LD (bios_vars.cursorY),A
        RET
editorScrollDown:
        LD HL,editorPageWidths+3
        LD DE,editorPageWidths+EDITOR_ROWS*3
        LD BC,editorPageWidths
        CALL bios_memcpy_bc_hl
        LD HL,editorRowLengths+1
        LD DE,editorRowLengths
        LD B,EDITOR_ROWS-1
editorLengthsUp:
        LD A,(HL)
        LD (DE),A
        INC HL
        INC DE
        DEC B
        JP NZ,editorLengthsUp
        LD A,(v_columns)
        LD (DE),A
        LD HL,(v_topLine)
        CALL nextLine
        LD (v_topLine),HL
        LD HL,09000h+EDITOR_TEXT_TOP+10
        LD BC,09000h+EDITOR_TEXT_TOP
        JP editorScrollRow
keyHandler_Up:
        CALL editorSaveVerticalGoal
        LD HL,editorFinishVerticalMove
        PUSH HL
        CALL previousCurrentLine
        RET Z
        LD A,(bios_vars.cursorY)
        CP EDITOR_FIRST_Y
        JP Z,editorScrollUp
        SUB 10
        LD (bios_vars.cursorY),A
        RET
editorScrollUp:
        LD HL,editorPageWidths
        LD DE,editorPageWidths+(EDITOR_ROWS-1)*3
        LD BC,editorPageWidths+3
        CALL bios_memcpy_bc_hl
        LD HL,editorRowLengths+EDITOR_ROWS-2
        LD DE,editorRowLengths+EDITOR_ROWS-1
        LD B,EDITOR_ROWS-1
editorLengthsDown:
        LD A,(HL)
        LD (DE),A
        DEC HL
        DEC DE
        DEC B
        JP NZ,editorLengthsDown
        LD A,(v_columns)
        LD (DE),A
        LD HL,(v_currentLine)
        LD (v_topLine),HL
        LD HL,09000h+EDITOR_TEXT_TOP
        LD BC,09000h+EDITOR_TEXT_TOP+10
editorScrollRow:
        COLOR COLOR_TEXT
        LD A,(editorCopyWidth)
        LD D,A
        LD E,EDITOR_HEIGHT-10
        CALL uiCopyTextRect
        JP editorRedrawLine

; Только видимый срез. RawLim выводит все байты как глифы, без CR/Tab-команд.
; Последняя ячейка допустима: после неё не печатаем следующий символ.
drawTextLine:
        LD A,1
        LD (editorWholeRow),A
        XOR A
        LD (editorSliceStart),A
        LD A,(v_columns)
        LD (editorSliceWidth),A
        JP drawTextRow
drawTextSlice:
        XOR A
        LD (editorWholeRow),A
drawTextRow:
        PUSH BC
        CALL disableInverse
        CALL editorRowSelection
        LD A,(editorSliceStart)
        LD C,A
        LD A,(v_charStep)
        LD B,A
        XOR A
drawSliceX:
        DEC C
        JP M,drawSliceXReady
        ADD A,B
        JP drawSliceX
drawSliceXReady:
        LD (bios_vars.cursorY+1),A
        LD A,(editorSliceWidth)
        LD C,A
        LD A,(editorSliceStart)
    IF EDIT_BANKED
        CALL ebPrepareRow
    ELSE
        CALL editorPrepareRow
    ENDIF
drawBufferComplete:
        ; C is the unused width; keep only the visible text length.
        PUSH HL
        PUSH DE
        LD A,(editorSliceWidth)
        SUB C
        LD (editorTextLength),A
        LD A,(editorWholeRow)
        OR A
        JP Z,drawSlicePadding
        LD A,(editorSliceWidth)
        SUB C
        LD B,A
        LD A,(editorSelectionRight)
        CP B
        JP C,editorSelectionLengthReady
        LD B,A
editorSelectionLengthReady:
        CALL editorRowLengthAddress
        LD A,(HL)
        LD (HL),B
        CP B
        JP NC,drawOldLengthReady
        LD A,B
drawOldLengthReady:
        ADD A,3
        AND 0FCh
        LD C,A
        LD A,(editorSliceWidth)
        CP C
        JP C,drawClampedLength
        LD A,C
drawClampedLength:
        LD (editorPaintLength),A
        LD C,A
        LD A,(editorTextLength)
        LD B,A
        LD A,C
        SUB B
        LD C,A
        JP drawPaddingReady
drawSlicePadding:
        LD A,(editorSliceWidth)
        LD (editorPaintLength),A
drawPaddingReady:
        POP DE
        POP HL
        LD A,C
        OR A
        JP Z,drawBufferReady
drawPaddingLoop:
        LD A,32
        LD (DE),A
        INC DE
        DEC C
        JP NZ,drawPaddingLoop
drawBufferReady:
        CALL nextLine
    IF EDIT_BANKED
        CALL ebObserveBottom
    ENDIF
        CALL editorRecordRowWidth
        PUSH HL
        PUSH AF
        LD HL,(bios_vars.cursorY)
        PUSH HL
        LD HL,editorRowBuffer
        LD A,(editorPartialRow)
        OR A
        JP NZ,editorPrintPartialRow
        ; One packet for normal text and its erased tail: splitting at an
        ; arbitrary character would force scalar heads/tails in 6px fonts.
        LD A,(editorPaintLength)
        OR A
        JP Z,editorRowPrinted
        LD C,A
        LD B,0
    IF EDIT_BANKED
        CALL ebPaintRow
    ELSE
        CALL bios_printStringRawLim
    ENDIF
        JP editorRowPrinted
editorPrintPartialRow:
        LD A,(editorEditStart)
        LD E,A
        LD A,(editorPaintLength)
        SUB E
        JP C,editorRowPrinted
        JP Z,editorRowPrinted
        PUSH AF
        LD D,0
        ADD HL,DE
        PUSH HL
        LD C,E
        LD A,(v_charStep)
        LD B,A
        XOR A
        INC C
editorPartialX:
        DEC C
        JP Z,editorPartialXReady
        ADD A,B
        JP editorPartialX
editorPartialXReady:
        LD (bios_vars.cursorY+1),A
        POP HL
        POP AF
        LD C,A
        LD B,0
        CALL bios_printStringRawLim
editorRowPrinted:
        POP HL
        LD (bios_vars.cursorY),HL
        CALL editorPaintSelection
        POP AF
        POP HL
        PUSH AF
        CALL disableInverse
        POP AF
        POP BC
        RET
    IF EDIT_BANKED
editorRowBuffer = EDIT_TAGS+EDIT_CACHE_MAX_ROWS*8
        ASSERT editorRowBuffer+96 <= EDIT_BANK_STATE
    ELSE
editorRowBuffer: DS 96,32
    ENDIF
editorRowLengths:   DS EDITOR_ROWS,0
editorWholeRow:     DB 0
editorPaintLength:  DB 0
editorTextLength:   DB 0
editorPartialRow:   DB 0
editorEditStart:    DB 0
; Return HL = cache entry for the current screen baseline; preserve BC/DE.
editorRowLengthAddress:
        PUSH BC
        LD A,(bios_vars.cursorY)
        SUB EDITOR_FIRST_Y
        LD C,0
editorRowIndex:
        SUB 10
        JP C,editorRowIndexDone
        INC C
        JP editorRowIndex
editorRowIndexDone:
        LD B,0
        LD HL,editorRowLengths
        ADD HL,BC
        POP BC
        RET
editorResetLengths:
        PUSH HL
        PUSH BC
        LD HL,editorRowLengths
        LD B,EDITOR_ROWS
editorResetLengthLoop:
        LD (HL),0
        INC HL
        DEC B
        JP NZ,editorResetLengthLoop
        POP BC
        POP HL
        RET
editorSliceStart:       DB 0
editorSliceWidth:       DB 0
editorPageRetained:     DB 0
testPositionSelected:
        PUSH DE
        EX DE,HL
        LD HL,(v_selectionBegin)
        LD A,H
        OR L
        JP Z,selectionTestDone
        CALL compareHLDE
        JP Z,selectionPositionEnd
        JP NC,selectionTestDone
selectionPositionEnd:
        LD HL,(v_selectionEnd)
        CALL compareHLDE
        JP Z,selectionPositionOutside
        CCF
        JP selectionTestDone
selectionPositionOutside:
        OR A
selectionTestDone:
        EX DE,HL
        POP DE
        RET

keyHandler_Right:
        CALL editorResetGoal
moveRightWithinLine:
        CALL editorLineLength
        EX DE,HL
        LD HL,(v_column)
        CALL compareHLDE
        JP C,editorMoveRightChar
        CALL keyHandler_Down
        RET Z
        JP keyHandler_Home
editorMoveRightChar:
        INC HL
        LD (v_column),HL
        JP editorPositionCursor
keyHandler_Left:
        CALL editorResetGoal
        LD HL,(v_column)
        LD A,H
        OR L
        JP Z,editorMovePreviousLine
        DEC HL
        LD (v_column),HL
        JP editorPositionCursor
editorMovePreviousLine:
        CALL keyHandler_Up
        RET Z
        JP keyHandler_End
keyHandler_End:
        CALL editorResetGoal
        CALL editorLineLength
        LD (v_column),HL
        JP editorPositionCursor
keyHandler_Home:
        CALL editorResetGoal
        LD HL,0
        LD (v_column),HL
        RET
keyHandler_Enter:
        CALL editorResetGoal
        CALL getLineBufferCursor
        CALL testPositionSelected
        JP C,Beep4
        CALL editorFreezeMark
        CALL commitLineEdit
        CALL editorClampToText
        CALL getLineBufferCursor
        LD D,H
        LD E,L
        INC DE
        PUSH HL
        CALL resizeTextRange
        POP HL
        ; Inserting CR before a lone LF would leave just one visible break.
        INC HL
        CALL gapRead
        DEC HL
        LD C,13
        CP 10
        JP NZ,editorWriteBreak
        LD C,10
editorWriteBreak:
        CALL gapWrite
        LD BC,1
        CALL editorAddLines
        CALL keyHandler_Down
        CALL keyHandler_Home
        JP redrawPreservingCursor

; HL=длина текущей строки, DE=её начало. Разделитель не включён.
editorLineLength:
        LD HL,(v_currentLine)
        LD D,H
        LD E,L
        CALL nextLine
        OR A
        JP Z,editorLengthReady
        DEC HL
        DEC A
        JP Z,editorLengthReady
        DEC HL
editorLengthReady:
        JP bios_sub_hl_de
getLineBufferCursor:
        PUSH BC
        CALL editorLineLength
        PUSH HL
        LD HL,(v_column)
        EX (SP),HL
        POP BC
        PUSH DE
        LD D,B
        LD E,C
        CALL compareHLDE
        JP C,editorCursorPointer
        EX DE,HL
editorCursorPointer:
        POP DE
        ADD HL,DE
        POP BC
        RET
editorClampToText:
        CALL editorLineLength
        EX DE,HL
        LD HL,(v_column)
        CALL compareHLDE
        RET C
        EX DE,HL
        LD (v_column),HL
        RET

;----------------------------------------------------------------------------
; MXOS
;
; Прибавить номер колонки курсора к адресу строки.
;
; вход:
;   hl = исходный адрес, v_column = 16-битное смещение.
;
; выход:
;   hl = сумма, cf = перенос за пределы слова. a, bc и de сохранены.
;----------------------------------------------------------------------------

addCursorColumn:
        PUSH DE
        EX DE,HL
        LD HL,(v_column)
        ADD HL,DE
        POP DE
        RET

; CF=не хватило памяти для снимка. Документ в этом случае не меняется.
beginLineEdit:
        PUSH BC
        LD A,(v_lineDirty)
        OR A
        JP Z,editorBeginSnapshot
        LD HL,(v_undoLine)
        EX DE,HL
        LD HL,(v_currentLine)
        CALL compareHLDE
        JP NZ,editorReplaceSnapshot
        POP BC
        RET
editorReplaceSnapshot:
        CALL commitLineEdit
editorBeginSnapshot:
        CALL editorLineLength
        PUSH HL
        CALL gapFree
        POP DE
        CALL compareHLDE
        JP C,editorUndoNoMemory
        JP Z,editorUndoNoMemory
        PUSH DE
        CALL getLineBufferCursor
        CALL gapMove
        POP DE
        PUSH DE
        LD HL,(v_gapHi)
        CALL bios_sub_hl_de
        LD (v_undoStart),HL
        EX DE,HL
        LD HL,(v_currentLine)
        POP BC
        PUSH BC
editorUndoCopy:
        LD A,B
        OR C
        JP Z,editorUndoSaved
        CALL gapRead
        LD (DE),A
        INC HL
        INC DE
        DEC BC
        JP editorUndoCopy
editorUndoSaved:
        POP HL
        LD (v_undoLength),HL
        LD HL,(v_column)
        LD (v_undoColumn),HL
        LD HL,(v_currentLine)
        LD (v_undoLine),HL
        LD HL,(v_lineNumber)
        LD (v_undoLineNumber),HL
        LD HL,(v_leftColumn)
        LD (v_undoLeftColumn),HL
        LD A,(v_leftColumnHi)
        LD (v_undoLeftColumnHi),A
        LD A,1
        LD (v_lineDirty),A
        OR A
        POP BC
        RET
editorUndoNoMemory:
        POP BC
        SCF
        RET

;----------------------------------------------------------------------------
; MXOS
;
; Принять текущие изменения строки и сбросить длину отмены.
;
; вход:
;   v_lineDirty и v_undoLength принадлежат активному сеансу редактора.
;
; выход:
;   Оба поля обнулены. a = 0, z = 1, cf = 0. bc, de и hl сохранены.
;----------------------------------------------------------------------------

commitLineEdit:
        XOR A
        LD (v_lineDirty),A
        PUSH HL
        LD HL,0
        LD (v_undoLength),HL
        POP HL
        RET

keyHandler_Space:
        CALL editorResetGoal
        CALL getLineBufferCursor
        CALL testPositionSelected
        JP C,Beep4
        CALL editorFreezeMark
        CALL beginLineEdit
        JP C,Beep4
        CALL editorRememberEditStart
        PUSH BC
        CALL getLineBufferCursor
        POP BC
        LD A,(v_insertMode)
        OR A
        JP NZ,editorInsertChar
        CALL gapTextRead
        OR A
        JP Z,editorInsertChar
        CP 13
        JP NZ,editorStoreChar
editorInsertChar:
        PUSH BC
        PUSH HL
        LD D,H
        LD E,L
        INC DE
        CALL resizeTextRange
        POP HL
        POP BC
editorStoreChar:
        CALL gapWrite
        CALL editorSetDirty
        LD HL,(v_column)
        INC HL
        LD (v_column),HL
        JP editorRedrawEditedLine

;----------------------------------------------------------------------------
; MXOS
;
; Переключить режим вставки и замены.
;
; вход:
;   v_insertMode содержит байт режима, 00h либо FFh.
;
; выход:
;   a и v_insertMode содержат инвертированный байт режима. bc, de, hl и флаги
;   сохранены.
;----------------------------------------------------------------------------

editorToggleInsert:
        LD A,(v_insertMode)
        CPL
        LD (v_insertMode),A
        RET
keyHandler_Backspace:
        LD HL,(v_selectionBegin)
        LD A,H
        OR L
        JP NZ,editorDeleteSelection
        CALL editorResetGoal
        CALL testShiftKey
        JP NZ,keyHandler_Del
        LD HL,(v_column)
        LD A,H
        OR L
        JP NZ,editorBackspaceChar
        CALL keyHandler_Up
        RET Z
        CALL keyHandler_End
        JP keyHandler_Del
editorBackspaceChar:
        DEC HL
        LD (v_column),HL
        JP keyHandler_Del
keyHandler_Del:
        LD HL,(v_selectionBegin)
        LD A,H
        OR L
        JP NZ,editorDeleteSelection
        CALL editorFreezeMark
        CALL editorResetGoal
        CALL getLineBufferCursor
        CALL testPositionSelected
        JP C,Beep4
        CALL getLineBufferCursor
        CALL gapTextRead
        OR A
        RET Z
        CP 13
        JP Z,editorDeleteSeparator
        CALL beginLineEdit
        JP C,Beep4
        CALL editorRememberEditStart
        CALL getLineBufferCursor
        LD D,H
        LD E,L
        INC HL
        CALL resizeTextRange
        CALL editorJoinAtGap
        JP NZ,editorRedrawEditedLine
        JP redrawPreservingCursor
editorDeleteSeparator:
        PUSH HL
        CALL editorSkipSeparator
        CALL testPositionSelected
        POP HL
        JP C,Beep4
        CALL commitLineEdit
        LD D,H
        LD E,L
        CALL editorSkipSeparator
        CALL resizeTextRange
        LD BC,1
        CALL editorSubtractLines
        CALL editorJoinAtGap
        JP redrawPreservingCursor

; Deleting the byte(s) between CR and LF merges two surrounding lines.
; Keep the caret at the next valid line start, including at the top edge.
editorJoinAtGap:
        LD HL,(v_gapLo)
        CALL editorInsideCRLF
        RET NZ
        PUSH HL
        LD HL,(v_topLine)
        EX DE,HL
        POP HL
        CALL compareHLDE
        INC HL
        JP NZ,editorJoinCaret
        LD (v_topLine),HL
editorJoinCaret:
        LD (v_currentLine),HL
        LD HL,0
        LD (v_column),HL
        LD BC,1
        CALL editorSubtractLines
        XOR A
        RET

editorUndoLine:
    IF EDIT_BANKED
        CALL ebInvalidateRows
    ENDIF
        CALL editorResetGoal
        LD A,(v_lineDirty)
        OR A
        RET Z
        CALL editorFreezeMark
        LD HL,(v_selectionBegin)
        LD A,H
        OR L
        CALL NZ,editorClearSelection
        LD HL,(v_undoLine)
        CALL editorInsideCRLF
        JP NZ,editorUndoOrdinaryLine
        PUSH HL
        LD BC,1
        CALL editorAddLines
        POP HL
        LD (v_currentLine),HL
        LD (v_topLine),HL
        LD A,EDITOR_FIRST_Y
        LD (bios_vars.cursorY),A
        LD A,1
        LD (editorUndoRelocated),A
        EX DE,HL
        LD HL,0
        JP editorUndoResize
editorUndoOrdinaryLine:
        CALL editorReturnToUndoLine
        CALL editorLineLength
editorUndoResize:
        ADD HL,DE
        PUSH HL
        LD HL,(v_undoLength)
        ADD HL,DE
        EX DE,HL
        POP HL
        CALL resizeTextRange
        LD HL,(v_currentLine)
        CALL compareHLDE
        JP Z,editorUndoDone
        PUSH HL
        LD HL,(v_undoStart)
        LD B,H
        LD C,L
        POP HL
editorUndoRestore:
        LD A,(BC)
        PUSH BC
        LD C,A
        CALL gapWrite
        POP BC
        INC BC
        INC HL
        CALL compareHLDE
        JP NZ,editorUndoRestore
editorUndoDone:
        LD HL,(v_undoLineNumber)
        LD (v_lineNumber),HL
        CALL commitLineEdit
        CALL editorSetDirty
        LD HL,(v_undoColumn)
        LD (v_column),HL
        CALL editorRestoreUndoWindow
        LD A,(editorUndoRelocated)
        OR A
        JP NZ,redrawPreservingCursor
        JP editorRedrawLine

; Keep the viewport when the saved line is visible; otherwise bring it to
; the first row. The snapshot belongs to a logical line, not the caret.
editorUndoRelocated: DB 0
editorRestoreUndoWindow:
        LD HL,(v_leftColumn)
        EX DE,HL
        LD HL,(v_undoLeftColumn)
        CALL compareHLDE
        LD (v_leftColumn),HL
        JP NZ,editorUndoWindowChanged
        LD A,(v_undoLeftColumnHi)
        LD B,A
        LD A,(v_leftColumnHi)
        CP B
        RET Z
editorUndoWindowChanged:
        LD A,(v_undoLeftColumnHi)
        LD (v_leftColumnHi),A
        LD A,1
        LD (editorUndoRelocated),A
        RET
editorReturnToUndoLine:
        XOR A
        LD (editorUndoRelocated),A
        LD HL,(v_currentLine)
        EX DE,HL
        LD HL,(v_undoLine)
        CALL compareHLDE
        RET Z
        LD (v_currentLine),HL
        EX DE,HL
        LD HL,(v_undoLineNumber)
        LD (v_lineNumber),HL
        LD A,1
        LD (editorUndoRelocated),A
editorRevealCurrentLine:
        LD HL,(v_currentLine)
        EX DE,HL
        LD HL,(v_topLine)
        LD B,EDITOR_ROWS
        LD C,EDITOR_FIRST_Y
editorUndoFindRow:
        CALL compareHLDE
        JP Z,editorUndoRowReady
        JP NC,editorUndoOutsideView
        CALL nextLine
        OR A
        JP Z,editorUndoOutsideView
        LD A,C
        ADD A,10
        LD C,A
        DEC B
        JP NZ,editorUndoFindRow
editorUndoOutsideView:
        EX DE,HL
        LD (v_topLine),HL
        LD C,EDITOR_FIRST_Y
editorUndoRowReady:
        LD A,C
        LD (bios_vars.cursorY),A
        RET

; Обычная правка обновляет только строку; горизонтальное окно меняется отдельно.
; Remember the old caret before mutation, including the byte removed by BS.
; Paint begins at the preceding group of four glyphs for the fast renderer.
editorRememberEditStart:
        PUSH BC
        PUSH DE
        PUSH HL
        CALL editorMeasureCursor
        LD B,A
        LD A,(v_leftColumnHi)
        LD C,A
        LD A,B
        CP C
        JP C,editorEditBeforeView
        PUSH HL
        LD HL,(v_leftColumn)
        EX DE,HL
        POP HL
        LD A,B
        CP C
        JP NZ,editorEditDistance
        CALL compareHLDE
        JP C,editorEditBeforeView
editorEditDistance:
        CALL bios_sub_hl_de
        LD A,B
        SBC A,C
        OR A
        JP NZ,editorEditAfterView
        LD A,H
        OR A
        JP NZ,editorEditAfterView
        LD A,(v_columns)
        CP L
        JP C,editorEditAfterView
        LD A,L
        AND 0FCh
        JP editorEditStartReady
editorEditBeforeView:
        XOR A
        JP editorEditStartReady
editorEditAfterView:
        LD A,(v_columns)
editorEditStartReady:
        LD (editorEditStart),A
        POP HL
        POP DE
        POP BC
        RET
editorRedrawEditedLine:
        LD A,1
        LD (editorPartialRow),A
        CALL editorRedrawLine
        XOR A
        LD (editorPartialRow),A
        RET
editorRedrawLine:
        CALL saveCursor
        COLOR COLOR_TEXT
        LD HL,(v_currentLine)
        CALL drawTextLine
        JP restoreCursor
; Вход: HL = прежняя граница, DE = новая граница хвоста документа.
; Выход: зазор поглощает/выдаёт байты, логические указатели обновлены.
; Разрушает AF, BC, HL, флаги; DE сохраняется на обычном пути.
; Расширение проверяет свободное место зазора с учётом снимка F8.
; Число строк обновляет вызывающая операция после изменения разделителей.
resizeTextRange:
    IF EDIT_BANKED
        CALL ebInvalidateRows
    ENDIF
        LD (gapOld),HL
        PUSH HL
        EX DE,HL
        LD (gapNew),HL
        EX DE,HL
        POP HL
        CALL compareHLDE
        RET Z
        PUSH AF
        CALL subtractHLFromDE
        PUSH HL
        EX DE,HL
        LD (gapDelta),HL
        EX DE,HL
        POP HL
        POP AF
        JP NC,gapResizeValidated
        CALL gapFree
        PUSH HL
        LD HL,(gapDelta)
        EX DE,HL
        POP HL
        CALL compareHLDE
        JP C,documentOutOfMemory
        JP NZ,gapResizeValidated
        LD HL,(v_undoLength)
        LD A,H
        OR L
        JP NZ,documentOutOfMemory
gapResizeValidated:
        LD HL,(gapOld)
        PUSH HL
        LD HL,(gapNew)
        EX DE,HL
        POP HL
        CALL compareHLDE
        JP NC,gapResizeDelete
        CALL gapMove
        LD HL,(v_gapLo)
        PUSH HL
        LD HL,(gapDelta)
        EX DE,HL
        POP HL
        ADD HL,DE
        LD (v_gapLo),HL
        JP gapResizeMoved
gapResizeDelete:
        EX DE,HL
        CALL gapMove
        LD HL,(v_gapHi)
        PUSH HL
        LD HL,(gapDelta)
        EX DE,HL
        POP HL
        CALL bios_sub_hl_de
        EX DE,HL
        PUSH DE
        CALL gapMoveUndo
        POP HL
        LD (v_gapHi),HL
gapResizeMoved:
        CALL gapSentinel
        LD HL,(v_textEnd)
        PUSH HL
        LD HL,(gapDelta)
        EX DE,HL
        POP HL
        ADD HL,DE
        LD (v_textEnd),HL
        LD B,D
        LD C,E
        PUSH HL
        LD HL,(gapOld)
        EX DE,HL
        POP HL
        LD HL,(v_selectionBegin)
        CALL compareHLDE
        JP C,adjustSelectionEnd
        ADD HL,BC
        LD (v_selectionBegin),HL
adjustSelectionEnd:
        LD HL,(v_selectionEnd)
        CALL compareHLDE
        JP C,gapResizeDone
        JP NZ,gapAdjustEnd
        LD A,B
        OR A
        JP P,gapResizeDone
gapAdjustEnd:
        ADD HL,BC
        LD (v_selectionEnd),HL
gapResizeDone:
        PUSH HL
        LD HL,(gapNew)
        EX DE,HL
        POP HL
        CALL editorSetDirty
        RET
