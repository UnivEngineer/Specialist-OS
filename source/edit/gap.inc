; Logical document pointers never include the gap. Only mutations move it.
; [gapLo,gapHi) is free, except the current line's F8 snapshot at its high end.
v_gapLo:        DW 0
v_gapHi:        DW 0
gapTarget:      DW 0
gapAmount:      DW 0
gapDelta:       DW 0
gapOld:         DW 0
gapNew:         DW 0

; Initialize from the exact logical end of the contiguous file bytes.
gapInitialize:
    IF EDIT_BANKED
        CALL ebInvalidateRows
    ENDIF
        LD HL,0
        LD (v_undoLength),HL
        LD HL,(v_textEnd)
        LD (v_gapLo),HL
        LD HL,(v_textLimitLo)
        DEC HL
        LD (v_gapHi),HL
        LD (HL),0
        LD HL,(v_gapLo)
        LD (HL),0
        RET

; HL logical -> physical. BC/DE preserved.
gapPhysical:
        PUSH DE
        PUSH HL
        LD HL,(v_gapLo)
        EX DE,HL
        POP HL
        CALL compareHLDE
        JP C,gapPhysicalDone
        PUSH HL
        LD HL,(v_gapHi)
        CALL nativeMath.sub_hl_de
        EX DE,HL
        POP HL
        ADD HL,DE
gapPhysicalDone:
        POP DE
        RET
; Physical -> logical; document bytes must be outside the gap.
gapLogical:
        PUSH DE
        PUSH HL
        LD HL,(v_gapHi)
        EX DE,HL
        POP HL
        CALL compareHLDE
        JP C,gapPhysicalDone
        PUSH HL
        LD HL,(v_gapLo)
        EX DE,HL
        CALL nativeMath.sub_hl_de
        EX DE,HL
        POP HL
        CALL nativeMath.sub_hl_de
        POP DE
        RET
; Physical NUL can be a document byte, the gap marker, or EOF.
; Return a display space for embedded NUL, preserving BC/DE. HL may
; cross the gap. Only the last reserved byte represents physical EOF.
gapTextZero:
        PUSH DE
        PUSH HL
        LD HL,(v_gapLo)
        EX DE,HL
        POP HL
        CALL compareHLDE
        JP NZ,gapTextCheckEnd
        LD HL,(v_gapHi)
gapTextCheckEnd:
        LD A,(HL)
        OR A
        JP NZ,gapTextZeroDone
        PUSH HL
        LD HL,(v_textLimitLo)
        DEC HL
        EX DE,HL
        POP HL
        CALL compareHLDE
        LD A,0
        JP Z,gapTextZeroDone
        LD A,32
gapTextZeroDone:
        POP DE
        RET

; Logical text lookup. Raw file operations and undo use gapRead.
gapTextRead:
        PUSH HL
        CALL gapPhysical
        LD A,(HL)
        CALL editorTextControl
        POP HL
        RET
editorTextControl:
        OR A
        CALL Z,gapTextZero
        CP 10
        RET NZ
        LD A,13
        RET

; HL points to a raw CR or LF. Advance over the complete delimiter.
; A returns its width (1 or 2), including when the gap splits CRLF.
editorSkipSeparator:
        CALL gapRead
        INC HL
        CP 13
        JP NZ,editorSingleSeparator
        CALL gapRead
        CP 10
        JP NZ,editorSingleSeparator
        INC HL
        LD A,2
        RET
editorSingleSeparator:
        LD A,1
        RET

; HL is a logical line start above DE (document beginning).
editorBackSeparator:
        DEC HL
        CALL gapRead
        CP 10
        RET NZ
        CALL compareHLDE
        RET Z
        DEC HL
        CALL gapRead
        CP 13
        RET Z
        INC HL
        RET
gapSentinel:
        LD HL,(v_gapLo)
        PUSH HL
        LD HL,(v_gapHi)
        EX DE,HL
        POP HL
        CALL compareHLDE
        RET Z
        LD (HL),0
        RET
; A = logical byte, all pairs preserved.
gapRead:
        PUSH HL
        CALL gapPhysical
        LD A,(HL)
        POP HL
        RET
; C = byte, all pairs preserved.
gapWrite:
    IF EDIT_BANKED
        CALL ebInvalidateRows
    ENDIF
        PUSH HL
        CALL gapPhysical
        LD (HL),C
        POP HL
        RET
; HL = free bytes, excluding the F8 snapshot. DE destroyed.
gapFree:
        CALL gapCapacity
        PUSH HL
        LD HL,(v_undoLength)
        EX DE,HL
        POP HL
        JP nativeMath.sub_hl_de
; Total width, including a snapshot that a committed edit can discard.
gapCapacity:
        LD HL,(v_gapHi)
        PUSH HL
        LD HL,(v_gapLo)
        EX DE,HL
        POP HL
        JP nativeMath.sub_hl_de

; Move the snapshot to end at DE. Snapshot size may be zero.
gapMoveUndo:
        LD HL,(v_undoLength)
        LD A,H
        OR L
        RET Z
        CALL subtractHLFromDE
        LD B,D
        LD C,E
        LD HL,(v_undoStart)
        PUSH HL
        LD HL,(v_gapHi)
        EX DE,HL
        POP HL
        PUSH BC
        CALL mon2_memcpy_bc_hl
        POP HL
        LD (v_undoStart),HL
        RET

; HL = logical target. No copy for navigation; called by mutations only.
; With an F8 snapshot, bounded chunks keep it out of the copy destination.
gapMove:
        LD (gapTarget),HL
        LD HL,(v_gapHi)
        EX DE,HL
        LD HL,(v_gapLo)
        CALL compareHLDE
        JP NZ,gapMoveLoop
        ; A full document has a zero-width gap: relocating it copies nothing.
        LD HL,(gapTarget)
        LD (v_gapLo),HL
        LD (v_gapHi),HL
        RET
gapMoveLoop:
        LD HL,(v_gapLo)
        PUSH HL
        LD HL,(gapTarget)
        EX DE,HL
        POP HL
        CALL compareHLDE
        JP Z,gapSentinel
        PUSH AF
        JP C,gapMoveRightDistance
        CALL nativeMath.sub_hl_de
        JP gapMoveDistanceReady
gapMoveRightDistance:
        CALL subtractHLFromDE
        EX DE,HL
gapMoveDistanceReady:
        LD (gapAmount),HL
        LD HL,(v_undoLength)
        LD A,H
        OR L
        JP Z,gapMoveChunkReady
        CALL gapFree
        PUSH HL
        LD HL,(gapAmount)
        EX DE,HL
        POP HL
        CALL compareHLDE
        JP NC,gapMoveChunkReady
        LD (gapAmount),HL
gapMoveChunkReady:
        POP AF
        JP C,gapMoveRight
        LD HL,(v_gapHi)
        PUSH HL
        LD HL,(gapAmount)
        EX DE,HL
        POP HL
        CALL nativeMath.sub_hl_de
        EX DE,HL
        CALL gapMoveUndo
        LD HL,(v_gapLo)
        PUSH HL
        LD HL,(gapAmount)
        EX DE,HL
        POP HL
        CALL nativeMath.sub_hl_de
        PUSH HL
        LD HL,(v_gapHi)
        EX DE,HL
        POP HL
        PUSH HL
        LD HL,(gapAmount)
        CALL subtractHLFromDE
        LD B,D
        LD C,E
        PUSH HL
        LD HL,(v_gapLo)
        EX DE,HL
        POP HL
        POP HL
        CALL mon2_memcpy_bc_hl
        LD HL,(v_gapLo)
        PUSH HL
        LD HL,(gapAmount)
        EX DE,HL
        POP HL
        CALL nativeMath.sub_hl_de
        LD (v_gapLo),HL
        LD HL,(v_gapHi)
        CALL nativeMath.sub_hl_de
        LD (v_gapHi),HL
        JP gapMoveLoop
gapMoveRight:
        LD HL,(v_gapHi)
        PUSH HL
        LD HL,(gapAmount)
        EX DE,HL
        POP HL
        ADD HL,DE
        EX DE,HL
        LD HL,(v_gapHi)
        PUSH HL
        LD HL,(v_gapLo)
        LD B,H
        LD C,L
        POP HL
        CALL mon2_memcpy_bc_hl
        LD HL,(v_gapHi)
        PUSH HL
        LD HL,(gapAmount)
        EX DE,HL
        POP HL
        ADD HL,DE
        EX DE,HL
        CALL gapMoveUndo
        LD HL,(v_gapLo)
        PUSH HL
        LD HL,(gapAmount)
        EX DE,HL
        POP HL
        ADD HL,DE
        LD (v_gapLo),HL
        LD HL,(v_gapHi)
        ADD HL,DE
        LD (v_gapHi),HL
        JP gapMoveLoop

; Bulk file operations use a contiguous prefix, without abandoning the gap.
gapFlatten:
        CALL commitLineEdit
        LD HL,(v_textEnd)
        CALL gapMove
        LD HL,(v_textEnd)
        LD (HL),0
        RET
