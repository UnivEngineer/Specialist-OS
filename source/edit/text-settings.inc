editorCycleTab:
        CALL editorAdvanceTabWidth
        CALL editorPositionCursor
        JP redrawPreservingCursor
editorToggleEncoding:
        LD A,(editorEncodingDigit)
        XOR '8'^'7'
        LD (editorEncodingDigit),A
    IF EDIT_BANKED
        CALL ebInvalidateRows
    ENDIF
        CALL drawStatusBar
        JP redrawPreservingCursor
editorAdvanceTabWidth:
    IF EDIT_BANKED
        CALL ebInvalidateRows
    ENDIF
        LD A,(v_tabWidth)
        ADD A,2
        CP 10
        JP C,editorTabWidthReady
        LD A,2
editorTabWidthReady:
        LD (v_tabWidth),A
        JP editorResetGoal
