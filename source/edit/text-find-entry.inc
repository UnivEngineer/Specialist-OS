editorFind:
        CALL editorFreezeMark
        CALL editorResetGoal
        LD DE,editorFindTitle
        LD HL,editorFindDescriptor
        CALL editorOpenInputModal
        LD HL,v_searchText
        LD D,H
        LD E,L
editorSearchEnd:
        LD A,(DE)
        OR A
        JP Z,editorSearchInput
        INC DE
        JP editorSearchEnd
editorSearchInput:
        LD A,(editorInputWidth)
        LD C,A
        LD B,1
        CALL lineEdit
        PUSH AF
        CALL editorCloseModal
        POP AF
        CP 13
        RET NZ
        LD A,(v_searchText)
        OR A
        RET Z
        CALL getLineBufferCursor
        JP editorSearchStart
editorFindNext:
        CALL editorFreezeMark
        LD A,(v_searchText)
        OR A
        JP Z,editorFind
        CALL editorResetGoal
        CALL getLineBufferCursor
        INC HL
editorSearchStart:
        PUSH HL
        LD HL,(v_textEnd)
        EX DE,HL
        POP HL
        CALL compareHLDE
        JP C,editorSearchStartReady
        JP Z,editorSearchStartReady
        LD HL,(v_textBeginLo)
editorSearchStartReady:
        LD (editorSearchStop),HL
        XOR A
        LD (editorSearchWrapped),A
editorSearchCandidate:
        PUSH HL
        LD HL,(v_textEnd)
        EX DE,HL
        POP HL
        CALL compareHLDE
        JP NZ,editorSearchCheckStop
        LD A,(editorSearchWrapped)
        OR A
        JP NZ,editorSearchMissing
        INC A
        LD (editorSearchWrapped),A
        LD HL,(v_textBeginLo)
editorSearchCheckStop:
        LD A,(editorSearchWrapped)
        OR A
        JP Z,editorSearchCompare
        PUSH HL
        LD HL,(editorSearchStop)
        EX DE,HL
        POP HL
        CALL compareHLDE
        JP Z,editorSearchMissing
    IF EDIT_BANKED
        JP editorSearchCompare
    ENDIF
