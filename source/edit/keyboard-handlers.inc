editorRequestNew:
        LD HL,editorNewDocument
        JP editorConfirmAction
editorRequestLoad:
        LD HL,editorLoadDocument
        JP editorConfirmAction
editorNewDocument:
        CALL commitLineEdit
        LD HL,FILE_NAME_HISTORY
        LD B,MAX_PATH
editorNewName:
        LD (HL),0
        INC HL
        DEC B
        JP NZ,editorNewName
        LD HL,(v_textBeginLo)
        LD (v_textEnd),HL
        LD (HL),0
        XOR A
        LD (v_documentDirty),A
        JP scanDocument
editorCopyMarked:
        XOR A
        LD (editorMarkActive),A
        CALL editorCopySelection
        JP editorToolbar
editorMoveMarked:
        XOR A
        LD (editorMarkActive),A
        CALL editorMoveSelection
        JP editorToolbar
editorDeleteMarked:
        LD HL,(v_selectionBegin)
        LD A,H
        OR L
        JP Z,keyHandler_Del
        XOR A
        LD (editorMarkActive),A
        CALL editorDeleteSelection
        JP editorToolbar

editorLoadDocument:
        CALL inputFileName
        LD A,(editorFileCancelled)
        OR A
        RET NZ
        CALL gapFlatten
        CALL openFile
        JP scanDocument

editorAppendDocument:
        CALL inputAppendName
        LD A,(editorFileCancelled)
        OR A
        RET NZ
        CALL gapFlatten
        CALL insertFile
        JP scanDocument
