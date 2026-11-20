keyInput2:
        LD C,A
        CP 8Ah
        JP Z,hexToggle
        CP 7Fh
        JP Z,keyHandler_Backspace
        CP 8Bh
        JP Z,keyHandler_CTP
        CP 8Ch
        RET Z
        CP 20h
        JP NC,keyHandler_Space
        LD HL,editorKeyTable
editorDispatchKey:
        LD A,(HL)
        OR A
        RET Z
        INC HL
        LD E,(HL)
        INC HL
        LD D,(HL)
        INC HL
        CP C
        JP NZ,editorDispatchKey
        PUSH DE
        RET
editorFunctionKey:
        ADD A,A
        LD E,A
        LD D,0
        CALL testShiftKey
        LD HL,editorMainKeys
        JP Z,editorFunctionTable
        LD HL,editorFileKeys
editorFunctionTable:
        ADD HL,DE
        LD E,(HL)
        INC HL
        LD D,(HL)
        PUSH DE
        RET
editorMainKeys:
        DW editorToggleInsert,editorHelp,editorFind,editorToggleMark
        DW editorCopyMarked,editorMoveMarked,editorFindNext,editorDeleteMarked
editorFileKeys:
        DW editorCycleTab,editorToggleEncoding,editorRequestNew,editorRequestLoad
        DW editorWriteDocument,editorAppendDocument,editorUnusedFunction,editorUnusedFunction
editorUnusedFunction:
        RET
editorKeyTable:
        DB 1Ah
        DW keyHandler_Down
        DB 19h
        DW keyHandler_Up
        DB 08h
        DW keyHandler_Left
        DB 18h
        DW keyHandler_Right
        DB 0Dh
        DW keyHandler_Enter
        DB 09h
        DW keyHandler_Tab
        DB 1Fh
        DW keyHandler_CTP
        DB 0Ch
        DW keyHandler_Home
        DB 0Ah
        DW keyHandler_End
        DB 0
