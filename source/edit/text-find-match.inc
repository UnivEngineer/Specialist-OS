editorSearchCompare:
        PUSH HL
        LD DE,v_searchText
editorSearchByte:
        LD A,(DE)
        OR A
        JP Z,editorSearchFound
        LD B,A
        CALL gapRead
        OR A
        JP Z,editorSearchFailedCandidate
        CP B
        JP NZ,editorSearchFailedCandidate
        INC HL
        INC DE
        JP editorSearchByte
editorSearchFailedCandidate:
        POP HL
        INC HL
        JP editorSearchCandidate
editorSearchFound:
        POP HL
        LD (editorFoundPointer),HL
        PUSH HL
        LD HL,(v_textBeginLo)
        EX DE,HL
        POP HL
editorSearchLineStart:
        CALL compareHLDE
        JP Z,editorSearchLineReady
        DEC HL
        CALL gapTextRead
        CP 13
        JP NZ,editorSearchLineStart
        INC HL
editorSearchLineReady:
        LD (v_currentLine),HL
        CALL recountCurrentLine
        LD HL,(v_currentLine)
        EX DE,HL
        LD HL,(editorFoundPointer)
        CALL nativeMath.sub_hl_de
        LD (v_column),HL
        CALL editorRevealCurrentLine
        CALL editorPositionCursor
        JP redrawPreservingCursor
editorSearchMissing:
        LD HL,editorSearchMissingText
        CALL editorInfoModal
        JP editorCloseModal
