; Byte offsets address the document. Display columns are 24-bit: a MAIN
; line consisting of tabs can exceed 65535 display cells. Tab stops: configurable 2/4/6/8 cells.
v_visualColumn:     DW 0
v_visualColumnHi:   DB 0
v_goalColumn:       DW 0
v_goalColumnHi:     DB 0
v_goalValid:        DB 0
editorCalcHi:       DB 0
editorRemainingHi:  DB 0
editorTabPending:   DB 0
v_tabWidth:         DB 4
editorTabLeft:      DB 4

;----------------------------------------------------------------------------
; MXOS
;
; Отменить запомненную целевую колонку вертикального движения.
;
; вход:
;   Регистровых параметров нет.
;
; выход:
;   v_goalValid и a равны 0, z = 1, cf = 0. bc, de и hl сохранены.
;----------------------------------------------------------------------------

editorResetGoal:
        XOR A
        LD (v_goalValid),A
        RET

; Measure the byte caret, stopping at the line break or EOF.
; Result A:HL and v_visualColumn[3]; BC/DE destroyed.
editorMeasureCursor:
        LD HL,(v_column)
        LD B,H
        LD C,L
        LD HL,(v_currentLine)
        CALL editorMeasureBytes
        LD (v_visualColumn),HL
        LD (v_visualColumnHi),A
        RET
; HL=line, BC=byte count -> A:HL=visual column, without moving the caret.
editorMeasureBytes:
        CALL gapPhysical
        LD DE,0
        XOR A
        LD (editorCalcHi),A
        LD A,(v_tabWidth)
        LD (editorTabLeft),A
editorMeasureLoop:
        LD A,B
        OR C
        JP Z,editorMeasureDone
        LD A,(HL)
        CP 14
        JP NC,editorMeasureValue
        CALL editorTextControl
        OR A
        JP Z,editorMeasureDone
        CP 13
        JP Z,editorMeasureDone
editorMeasureValue:
        CP 9
        JP Z,editorMeasureTab
        LD A,(editorTabLeft)
        DEC A
        JP NZ,editorMeasurePhase
        LD A,(v_tabWidth)
editorMeasurePhase:
        LD (editorTabLeft),A
        INC E
        JP NZ,editorMeasureNext
        JP editorMeasureCarry
editorMeasureTab:
        LD A,(editorTabLeft)
        ADD A,E
        LD E,A
        LD A,(v_tabWidth)
        LD (editorTabLeft),A
        JP NC,editorMeasureNext
editorMeasureCarry:
        INC D
        JP NZ,editorMeasureNext
        LD A,(editorCalcHi)
        INC A
        LD (editorCalcHi),A
editorMeasureNext:
        INC HL
        DEC BC
        JP editorMeasureLoop
editorMeasureDone:
        EX DE,HL
        LD A,(editorCalcHi)
        RET

editorSaveVerticalGoal:
        PUSH AF
        PUSH BC
        PUSH DE
        PUSH HL
        LD A,(v_goalValid)
        OR A
        JP NZ,editorGoalSaved
        CALL editorClampToText
        CALL editorMeasureCursor
        LD (v_goalColumn),HL
        LD (v_goalColumnHi),A
        LD A,1
        LD (v_goalValid),A
editorGoalSaved:
        POP HL
        POP DE
        POP BC
        POP AF
        RET

; Return trampoline for vertical handlers; preserve their Z/NZ result.
editorFinishVerticalMove:
        PUSH AF
        PUSH BC
        PUSH DE
        PUSH HL
        CALL editorGoalToByte
        POP HL
        POP DE
        POP BC
        POP AF
        RET

; Map the desired display column onto the preceding legal byte boundary.
editorGoalToByte:
        LD A,(v_tabWidth)
        LD (editorTabLeft),A
        LD HL,(v_goalColumn)
        EX DE,HL
        LD A,(v_goalColumnHi)
        LD (editorRemainingHi),A
        LD HL,(v_currentLine)
        CALL gapPhysical
        LD BC,0
editorGoalLoop:
        LD A,D
        OR E
        JP NZ,editorGoalRead
        LD A,(editorRemainingHi)
        OR A
        JP Z,editorGoalDone
editorGoalRead:
        LD A,(HL)
        CP 14
        JP NC,editorGoalValue
        CALL editorTextControl
        OR A
        JP Z,editorGoalDone
        CP 13
        JP Z,editorGoalDone
editorGoalValue:
        CP 9
        JP Z,editorGoalTab
        LD A,1
        JP editorGoalConsume
editorGoalTab:
        LD A,(editorTabLeft)
editorGoalConsume:
        LD (editorTabPending),A
        LD A,(editorRemainingHi)
        OR A
        JP NZ,editorGoalEnough
        LD A,D
        OR A
        JP NZ,editorGoalEnough
        LD A,(editorTabPending)
        CP E
        JP Z,editorGoalEnough
        JP NC,editorGoalDone
editorGoalEnough:
        LD A,(editorTabPending)
        PUSH BC
        LD C,A
        LD A,(editorTabLeft)
        SUB C
        JP NZ,editorGoalPhase
        LD A,(v_tabWidth)
editorGoalPhase:
        LD (editorTabLeft),A
        LD A,E
        SUB C
        LD E,A
        LD A,D
        SBC A,0
        LD D,A
        LD A,(editorRemainingHi)
        SBC A,0
        LD (editorRemainingHi),A
        POP BC
        INC HL
        INC BC
        JP editorGoalLoop
editorGoalDone:
        LD H,B
        LD L,C
        LD (v_column),HL
        RET

; Prepare exactly the requested visual slice in the existing 96-byte buffer.
; HL=logical line, A=slice start relative to viewport, C=width.
; Return HL=logical text iterator, DE=buffer end, C=unused width, B destroyed.
; Skip whole tabs before the slice; expand only the visible part into spaces.
editorPrepareRow:
        PUSH HL
        LD E,A
        LD D,0
        LD HL,(v_leftColumn)
        ADD HL,DE
        LD A,(v_leftColumnHi)
        ADC A,0
        LD (editorRemainingHi),A
        EX DE,HL
        POP HL
        CALL gapPhysical
        XOR A
        LD (editorTabPending),A
        LD A,(v_tabWidth)
        LD B,A
editorPrepareSkip:
        LD A,D
        OR E
        JP NZ,editorPrepareSkipRead
        LD A,(editorRemainingHi)
        OR A
        JP Z,editorPrepareVisible
editorPrepareSkipRead:
        LD A,(HL)
        CP 14
        JP NC,editorPrepareSkipValue
        CALL editorTextControl
        OR A
        JP Z,editorPrepareVisible
        CP 13
        JP Z,editorPrepareVisible
editorPrepareSkipValue:
        CP 9
        JP Z,editorPrepareSkipTab
        INC HL
        DEC B
        JP NZ,editorPrepareSkipPhase
        LD A,(v_tabWidth)
        LD B,A
editorPrepareSkipPhase:
        LD A,D
        OR E
        JP NZ,editorPrepareSkipOne
        LD A,(editorRemainingHi)
        DEC A
        LD (editorRemainingHi),A
editorPrepareSkipOne:
        DEC DE
        JP editorPrepareSkip
editorPrepareSkipTab:
        LD A,B
        LD (editorTabPending),A
        LD A,(editorRemainingHi)
        OR A
        JP NZ,editorPrepareSkipTabFull
        LD A,D
        OR A
        JP NZ,editorPrepareSkipTabFull
        LD A,(editorTabPending)
        CP E
        JP C,editorPrepareSkipTabFull
        JP Z,editorPrepareSkipTabFull
        SUB E
        LD (editorTabPending),A
        LD B,A
        INC HL
        JP editorPrepareVisible
editorPrepareSkipTabFull:
        LD A,(editorTabPending)
        PUSH BC
        LD C,A
        LD A,E
        SUB C
        LD E,A
        LD A,D
        SBC A,0
        LD D,A
        LD A,(editorRemainingHi)
        SBC A,0
        LD (editorRemainingHi),A
        POP BC
        LD A,(v_tabWidth)
        LD B,A
        XOR A
        LD (editorTabPending),A
        INC HL
        JP editorPrepareSkip
editorPrepareVisible:
        LD DE,editorRowBuffer
        LD A,(editorTabPending)
        OR A
        JP NZ,editorPrepareSpace
editorPrepareCopy:
        LD A,(HL)
        CP 14
        JP NC,editorPrepareCopyValue
        CALL editorTextControl
        OR A
        JP Z,editorPrepareDone
        CP 13
        JP Z,editorPrepareDone
editorPrepareCopyValue:
        INC HL
        CP 9
        JP NZ,editorPrepareCharacter
        LD A,B
        LD (editorTabPending),A
editorPrepareSpace:
        DEC A
        LD (editorTabPending),A
        LD A,32
        LD (DE),A
        INC DE
        DEC B
        DEC C
        JP Z,editorPrepareDone
        LD A,B
        OR A
        JP NZ,editorPrepareSpace
        LD A,(v_tabWidth)
        LD B,A
        JP editorPrepareCopy
editorPrepareCharacter:
        CP 32
        JP C,editorPrepareControl
        CP 127
        JP NZ,editorPrepareGlyph
        LD A,(editorEncodingDigit)
        CP '7'
        LD A,127
        JP Z,editorPrepareGlyph
editorPrepareControl:
        LD A,32
editorPrepareGlyph:
        LD (DE),A
        INC DE
        DEC B
        JP NZ,editorPrepareGlyphPhase
        LD A,(v_tabWidth)
        LD B,A
editorPrepareGlyphPhase:
        DEC C
        JP NZ,editorPrepareCopy
        JP editorPrepareDone
editorPrepareDone:
        LD A,(editorEncodingDigit)
        CP '7'
        CALL Z,editorRenderKoi7
        JP gapLogical
; Remap the prepared glyphs only. Tabs and logical document offsets stay put.
editorRenderKoi7:
        PUSH HL
        LD HL,editorRowBuffer
        LD A,E
        SUB L
        JP Z,editorKoi7Done
        LD B,A
editorKoi7Glyph:
        LD A,(HL)
        CP 60h
        JP C,editorKoi7Next
        OR 80h
        LD (HL),A
editorKoi7Next:
        INC HL
        DEC B
        JP NZ,editorKoi7Glyph
editorKoi7Done:
        POP HL
        RET

; Six decimal cells, right aligned; no wrap at 65535 visual columns.
editorNumberLow:        DW 0
editorNumberHigh:       DB 0
editorNumberDiv:        DW 0
editorNumberOut:        DW 0
editorNumberCount:      DB 0
editorNumberLeading:    DB 0
editorNumberBuffer:     DS 6,32
                    DB 0 ; terminator for centered status values
editorNumberPowers: DB 0A0h,086h,1, 010h,027h,0, 0E8h,3,0
                    DB 100,0,0, 10,0,0, 1,0,0
editorFormatColumn:
        LD (editorNumberLow),HL
        LD (editorNumberHigh),A
        LD HL,editorNumberPowers
        LD (editorNumberDiv),HL
        LD HL,editorNumberBuffer
        LD (editorNumberOut),HL
        LD A,6
        LD (editorNumberCount),A
        XOR A
        LD (editorNumberLeading),A
editorNumberDigit:
        LD HL,(editorNumberDiv)
        LD E,(HL)
        INC HL
        LD D,(HL)
        INC HL
        LD C,(HL)
        INC HL
        LD (editorNumberDiv),HL
        LD B,'0'
editorNumberSubtract:
        LD HL,(editorNumberLow)
        LD A,(editorNumberHigh)
        CP C
        JP C,editorNumberEmit
        JP NZ,editorNumberTake
        CALL compareHLDE
        JP C,editorNumberEmit
editorNumberTake:
        CALL bios_sub_hl_de
        LD (editorNumberLow),HL
        LD A,(editorNumberHigh)
        SBC A,C
        LD (editorNumberHigh),A
        INC B
        JP editorNumberSubtract
editorNumberEmit:
        LD A,B
        CP '0'
        JP NZ,editorNumberNonzero
        LD A,(editorNumberLeading)
        OR A
        JP NZ,editorNumberStore
        LD A,(editorNumberCount)
        DEC A
        JP Z,editorNumberStore
        LD B,32
        JP editorNumberStore
editorNumberNonzero:
        LD A,1
        LD (editorNumberLeading),A
editorNumberStore:
        LD HL,(editorNumberOut)
        LD (HL),B
        INC HL
        LD (editorNumberOut),HL
        LD A,(editorNumberCount)
        DEC A
        LD (editorNumberCount),A
        JP NZ,editorNumberDigit
        RET
