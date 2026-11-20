; Forward, case-sensitive byte search. F3 opens a modal, F7 repeats and
; wraps once. Logical positions work on both sides of the document gap.
    IF !EDIT_BANKED
    INCLUDE "text-settings.inc"
    ENDIF
    IF !EDIT_BANKED
    INCLUDE "text-find-entry.inc"
    INCLUDE "text-find-match.inc"
    ENDIF
editorSearchMissingText: DB 'Text not found.',0
editorFindTitle: DB 'Find',0
editorFindButton: DB 'Find [Enter]',0
editorFindDescriptor:
        UI_MODAL_BEGIN 12,1,2
        UI_MODAL_BUTTON editorFindButton,0,0,1
        UI_MODAL_BUTTON editorModalCancel,0,1,0
        UI_MODAL_END
editorSearchStop: DW 0
editorSearchWrapped: DB 0
editorFoundPointer: DW 0

; Вход: HL = указатель в тексте. Выход: HL = начало предыдущей строки.
; Z = достигнуто начало текста, NZ = найден переход; DE сохранён, AF разрушен.
previousLine:
        PUSH DE
        EX DE,HL
        LD HL,(v_textBeginLo)
        EX DE,HL
        CALL compareHLDE
        JP Z,previousLineReturn
        CALL editorBackSeparator
        CALL compareHLDE
        JP Z,previousLineFound
        DEC HL
        CALL gapPhysical
        PUSH HL
        LD HL,(v_gapHi)
        EX DE,HL
        POP HL
        CALL compareHLDE
        JP NC,findPreviousLineLoop
        PUSH HL
        LD HL,(v_textBeginLo)
        EX DE,HL
        POP HL
findPreviousLineLoop:
        LD A,(HL)
        CP 10
        JP Z,previousLineAfterCR
        CP 13
        JP Z,previousLineAfterCR
        CALL compareHLDE
        JP Z,previousLineFloor
        DEC HL
        JP findPreviousLineLoop
previousLineFloor:
        PUSH HL
        LD HL,(v_textBeginLo)
        CALL compareHLDE
        EX DE,HL
        POP HL
        JP Z,previousLineConvert
        LD HL,(v_gapLo)
        CALL compareHLDE
        JP Z,previousLineFound
        DEC HL
        JP findPreviousLineLoop
previousLineAfterCR:
        INC HL
previousLineConvert:
        CALL gapLogical
previousLineFound:
        XOR A
        INC A
previousLineReturn:
        POP DE
        RET

editorCopySelection:
        XOR A
        JP editorBlockInsert
editorMoveSelection:
        LD A,1
editorBlockInsert:
        LD (editorBlockMove),A
        LD HL,(v_selectionBegin)
        LD A,H
        OR L
        JP Z,Beep4
        EX DE,HL
        LD HL,(v_selectionEnd)
        CALL bios_sub_hl_de
        LD (editorBlockLength),HL
        CALL getLineBufferCursor
        LD (editorBlockTarget),HL
        ; Insertion inside the source is ambiguous; both edges are legal.
        PUSH HL
        LD HL,(v_selectionBegin)
        EX DE,HL
        POP HL
        CALL compareHLDE
        JP C,editorBlockOutside
        JP Z,editorBlockEdge
        PUSH HL
        LD HL,(v_selectionEnd)
        EX DE,HL
        POP HL
        CALL compareHLDE
        JP C,Beep4
        JP NZ,editorBlockOutside
editorBlockEdge:
        LD A,(editorBlockMove)
        OR A
        RET NZ
editorBlockOutside:
        CALL editorCopyBlockBytes
        LD (editorBlockResult),HL
        EX DE,HL
        LD HL,(editorBlockTarget)
        CALL editorRangeBreaks
        CALL editorAddLines
        LD A,(editorBlockMove)
        OR A
        JP Z,editorBlockReposition
        ; Removing the source can join another pair at either boundary.
        LD HL,(v_selectionBegin)
        EX DE,HL
        LD HL,(editorBlockTarget)
        CALL compareHLDE
        JP C,editorBlockDeleteSource
        LD (editorBlockResult),HL
editorBlockDeleteSource:
        LD HL,(v_selectionBegin)
        CALL gapMove
        CALL editorRemoveCountedSelection
        LD HL,(editorBlockResult)
        LD (v_selectionEnd),HL
        PUSH HL
        LD HL,(editorBlockLength)
        EX DE,HL
        POP HL
        CALL bios_sub_hl_de
        LD (v_selectionBegin),HL
        JP editorBlockReposition

; Shared byte operations. The caller owns range validation and geometry.
editorCopyBlockBytes:
        CALL editorGrowBlock
        LD HL,(v_selectionBegin)
        LD B,H
        LD C,L
        LD HL,(editorBlockTarget)
        JP copyUntilDE
editorGrowBlock:
        CALL commitLineEdit
        LD HL,(editorBlockLength)
        EX DE,HL
        LD HL,(editorBlockTarget)
        PUSH HL
        ADD HL,DE
        EX DE,HL
        POP HL
        PUSH DE
        CALL resizeTextRange
        POP DE
        LD HL,(editorBlockTarget)
        RET
; Put both ranges before the gap. BIOS memmove handles overlap in either direction.
editorOverwriteBlockBytes:
        CALL commitLineEdit
        LD HL,(editorBlockTarget)
        EX DE,HL
        LD HL,(editorBlockLength)
        ADD HL,DE
        PUSH HL
        EX DE,HL
        LD HL,(v_selectionEnd)
        CALL compareHLDE
        JP NC,editorOverwriteMoveGap
        EX DE,HL
editorOverwriteMoveGap:
        CALL gapMove
        LD HL,(editorBlockTarget)
        LD B,H
        LD C,L
        LD HL,(v_selectionEnd)
        EX DE,HL
        LD HL,(v_selectionBegin)
        CALL bios_memcpy_bc_hl
        CALL editorSetDirty
        POP HL
        RET
; HL..DE (exclusive) is contiguous. Write the low byte of BC first.
editorFillBlockBytes:
        LD (HL),C
        INC HL
        LD A,B
        LD B,C
        LD C,A
        CALL compareHLDE
        JP NZ,editorFillBlockBytes
        RET
editorBlockReposition:
        LD HL,(editorBlockResult)
        CALL editorInsideCRLF
        JP NZ,editorBlockBoundary
        INC HL
editorBlockBoundary:
        EX DE,HL
        LD HL,(v_textBeginLo)
        LD (v_currentLine),HL
        LD HL,1
        LD (v_lineNumber),HL
editorBlockFindLine:
        LD HL,(v_currentLine)
        CALL nextLine
        OR A
        JP Z,editorBlockCaret
        CALL compareHLDE
        JP C,editorBlockNextLine
        JP NZ,editorBlockCaret
editorBlockNextLine:
        LD (v_currentLine),HL
        LD HL,(v_lineNumber)
        INC HL
        LD (v_lineNumber),HL
        JP editorBlockFindLine
editorBlockCaret:
        LD HL,(v_currentLine)
        LD (v_topLine),HL
        EX DE,HL
        CALL bios_sub_hl_de
        LD (v_column),HL
        LD A,EDITOR_FIRST_Y
        LD (bios_vars.cursorY),A
        CALL editorResetGoal
        CALL editorPositionCursor
        JP redrawPreservingCursor
editorBlockTarget: DW 0
editorBlockLength: DW 0
editorBlockResult: DW 0
editorBlockMove: DB 0
