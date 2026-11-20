; Exact byte selection, rendered as a pixel mask over aligned raw text.
; Tabs inherit their byte's selection; a selected CR highlights the row tail.
editorRowSelection:
        PUSH HL
        PUSH BC
        PUSH DE
        LD (editorSelectionRow),HL
        XOR A
        LD (editorSelectionLeft),A
        LD (editorSelectionRight),A
        LD HL,(v_selectionBegin)
        LD A,H
        OR L
        JP Z,editorSelectionDone
        EX DE,HL
        LD HL,(editorSelectionRow)
        CALL nextLine
        LD (editorSelectionTerm),A
        LD (editorSelectionLimit),HL
        CALL compareHLDE
        JP C,editorSelectionDone
        JP Z,editorSelectionDone
        LD HL,(v_selectionEnd)
        EX DE,HL
        LD HL,(editorSelectionRow)
        CALL compareHLDE
        JP NC,editorSelectionDone
        LD HL,(editorSelectionRow)
        EX DE,HL
        LD HL,(v_selectionBegin)
        CALL compareHLDE
        JP C,editorSelectionEndColumn
        CALL editorSelectionColumn
        LD (editorSelectionLeft),A
editorSelectionEndColumn:
        LD HL,(editorSelectionLimit)
        EX DE,HL
        LD HL,(v_selectionEnd)
        CALL compareHLDE
        JP C,editorSelectionEndInside
        LD A,(editorSelectionTerm)
        OR A
        LD A,(v_columns)
        JP NZ,editorSelectionEndReady
        EX DE,HL
editorSelectionEndInside:
        CALL editorSelectionColumn
editorSelectionEndReady:
        LD B,A
        LD A,(editorSelectionLeft)
        CP B
        JP NC,editorSelectionDone
        LD A,B
        LD (editorSelectionRight),A
editorSelectionDone:
        POP DE
        POP BC
        POP HL
        RET
; HL=logical boundary in this row -> clipped visual column in the viewport.
editorSelectionColumn:
        PUSH HL
        LD HL,(editorSelectionRow)
        EX DE,HL
        POP HL
        CALL bios_sub_hl_de
        LD B,H
        LD C,L
        LD HL,(editorSelectionRow)
        CALL editorMeasureBytes
        LD B,A
        EX DE,HL
        LD HL,(v_leftColumn)
        EX DE,HL
        CALL bios_sub_hl_de
        LD A,(v_leftColumnHi)
        LD C,A
        LD A,B
        SBC A,C
        JP M,editorSelectionBeforeWindow
        OR A
        JP NZ,editorSelectionAfterWindow
        LD A,H
        OR A
        JP NZ,editorSelectionAfterWindow
        LD A,(v_columns)
        CP L
        RET C
        LD A,L
        RET
editorSelectionBeforeWindow:
        XOR A
        RET
editorSelectionAfterWindow:
        LD A,(v_columns)
        RET
editorPaintSelection:
        PUSH AF
        PUSH BC
        PUSH DE
        PUSH HL
        LD A,(editorSelectionRight)
        OR A
        JP Z,editorSelectionPaintDone
        LD A,(editorSelectionLeft)
        LD B,A
        LD A,(editorSliceStart)
        LD C,A
        LD A,(editorPartialRow)
        OR A
        JP Z,editorSelectionPaintStart
        LD A,(editorEditStart)
        LD C,A
editorSelectionPaintStart:
        LD A,B
        CP C
        JP NC,editorSelectionPaintEnd
        LD B,C
editorSelectionPaintEnd:
        LD A,(editorSliceStart)
        LD C,A
        LD A,(editorPaintLength)
        ADD A,C
        LD C,A
        LD A,(editorSelectionRight)
        CP C
        JP C,editorSelectionPaintWidth
        LD A,C
editorSelectionPaintWidth:
        SUB B
        JP C,editorSelectionPaintDone
        JP Z,editorSelectionPaintDone
        LD L,A
        LD H,0
        LD A,B
        PUSH AF
        CALL editorSelectionPixels
        LD B,H
        LD C,L
        POP AF
        PUSH BC
        LD L,A
        LD H,0
        CALL editorSelectionPixels
        POP BC
        LD A,(bios_vars.cursorY)
        SUB 7
        LD D,A
        LD E,8
        CALL uiInverseRectPixels
editorSelectionPaintDone:
        POP HL
        POP DE
        POP BC
        POP AF
        RET
editorSelectionPixels:
        EX DE,HL
        LD HL,0
        LD A,(v_charStep)
        ADD A,A
        LD B,A
editorSelectionPixelLoop:
        ADD HL,DE
        DEC B
        JP NZ,editorSelectionPixelLoop
        RET
editorSelectionRow: DW 0
editorSelectionLimit: DW 0
editorSelectionTerm: DB 0
editorSelectionLeft: DB 0
editorSelectionRight: DB 0
