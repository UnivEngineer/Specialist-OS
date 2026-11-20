; Toolbars follow Shift. Each occupies exactly 48 bitmap columns.
editorUpdateToolbarKeys:
        CALL testShiftKey
        LD HL,editorShiftState
        CP (HL)
        RET Z
        LD (HL),A
editorToolbar:
        LD A,(hexActive)
        OR A
        JP NZ,hexToolbar
        LD HL,editorToolbarMarkLabel
        LD DE,editorToolbarMark
        LD A,(editorMarkActive)
        OR A
        JP Z,editorToolbarFrozen
        LD DE,editorToolbarEnd
        JP editorToolbarCopyMark
editorToolbarFrozen:
        PUSH HL
        LD HL,(v_selectionBegin)
        LD A,H
        OR L
        POP HL
        JP Z,editorToolbarCopyMark
        LD DE,editorToolbarUnmark
editorToolbarCopyMark:
        CALL editorToolbarCopyLabel
        LD HL,editorToolbarMain
        LD A,(editorShiftState)
        OR A
        JP Z,editorToolbarReady
        LD HL,editorToolbarFiles
editorToolbarReady:
        LD B,COLOR_HELP_F
        LD C,COLOR_STATUSBAR
        CALL uiToolbar
        COLOR COLOR_TEXT
        RET
editorToolbarCopyLabel:
        LD B,4
editorToolbarCopyByte:
        LD A,(DE)
        LD (HL),A
        INC DE
        INC HL
        DEC B
        JP NZ,editorToolbarCopyByte
        RET
    MACRO E_KEY KEY,TEXT
        UI_TOOLBAR_FIELD 2,0,KEY
        UI_TOOLBAR_FIELD 4,080h,TEXT
    ENDM
editorToolbarMain:
        E_KEY 'F1','Ins'
        E_KEY 'F2','Help'
        E_KEY 'F3','Find'
        UI_TOOLBAR_FIELD 2,0,'F4'
        DB 084h
editorToolbarMarkLabel: DB 'Mark',0
        E_KEY 'F5','Copy'
        E_KEY 'F6','Move'
        E_KEY 'F7','Next'
        E_KEY 'F8','Del '
        DB 0
editorToolbarFiles:
        E_KEY 'F1','Tab'
        E_KEY 'F2','KOI'
        E_KEY 'F3','New'
        E_KEY 'F4','Load'
        E_KEY 'F5','Save'
        E_KEY 'F6','App'
        UI_TOOLBAR_FIELD 2,0,'F9'
        UI_TOOLBAR_FIELD 3,080h,'Hex'
        UI_TOOLBAR_FIELD 3,0,'F10'
        UI_TOOLBAR_FIELD 4,080h,'Exit'
        DB 0
editorToolbarMark: DB 'Mark'
editorToolbarEnd: DB 'End '
editorToolbarUnmark: DB 'Unmk'

editorHelp:
        LD HL,editorHelpPath
        CALL uiShowHelp
        JP NC,editorHelpReturn
        LD HL,editorHelpError
        CALL editorInfoModal
        CALL editorCloseModal
editorHelpReturn:
        LD A,(hexActive)
        OR A
        RET NZ
        CALL editorPositionCursor
        JP updateStatusBar
    IF !EDIT_BANKED
editorHelpColor:
        COLOR 007h
        RET
    ENDIF
    DB COLOR_TEXT ; Background palette passed with the FAR help path.
editorHelpPath: DB 'A:',05Ch,'EDIT.HLP',0
editorHelpError: DB 'Cannot read A: EDIT.HLP',0


editorInfoModal:
        PUSH HL
        LD HL,editorInfoDescriptor
        CALL editorModalBegin
        COLOR 007h
        POP HL
        CALL uiModalTitle
editorInfoWait:
        CALL uiModalGetch
        CP 13
        RET Z
        CP 27
        JP NZ,editorInfoWait
        RET

; Show the original author's credit before file I/O and document rendering.
editorLoadingBanner:
        COLOR COLOR_BIOS
        CALL disableInverse
        LD HL,editorCopyright
        JP bios_printString
    MACRO EDIT_STARTUP_CREDIT
editorCopyright:
        DB "EDIT 6.0 (C) 2026 SPb SpaceEngineer",13,10
        DB "Based on E 4.1 (C) 1992 Omsk KSOFT",13,10,0
    ENDM

; HL = notice text. The size was checked by the file operation.
; Preserve HL/DE/BC for the pending read. The modal takes no input.
editorShowFileProgress:
        LD A,(fileDescSize+1)
        CP 10h
        RET C
        JP NZ,editorDrawFileProgress
        LD A,(fileDescSize)
        OR A
        RET Z
editorDrawFileProgress:
        PUSH BC
        PUSH DE
        PUSH HL
        PUSH HL
        LD HL,editorProgressDescriptor
        CALL editorModalBegin
        POP HL
        CALL uiModalTitle
    IF EDIT_BANKED
        CALL ebFlush
    ENDIF
        LD A,1
        LD (editorProgressVisible),A
        POP HL
        POP DE
        POP BC
        RET
editorFinishFileProgress:
        XOR A
        LD HL,editorProgressVisible
        CP (HL)
        RET Z
        LD (HL),A
        LD A,(editorInterfaceReady)
        OR A
        RET Z
        JP editorCloseModal
editorProgressDescriptor:
        UI_MODAL_NOTICE
editorProgressVisible: DB 0
editorBusyLoading: DB 'Loading...',0
editorBusySaving: DB 'Saving...',0

; SDK owns the frame, separators, button geometry and text alignment.
editorModalBegin:
        PUSH HL
        LD HL,(bios_vars.cursorY)
        LD (editorModalCursor),HL
        POP HL
        CALL disableInverse
        COLOR 007h
        LD A,(FONT_ENGINE_HEADER+5)
        JP uiModalOpen
editorErrorModal:
        LD DE,editorErrorTitle
; HL = explanation, DE = title.
editorErrorWithTitle:
        LD A,(editorInterfaceReady)
        OR A
        JP Z,editorStartupError
        PUSH HL
        PUSH DE
        LD HL,editorErrorDescriptor
        CALL editorModalBegin
        COLOR 047h
        POP HL
        CALL uiModalTitle
        POP HL
        CALL uiModalBodyText
editorErrorWait:
        CALL uiModalGetch
        CP 13
        RET Z
        CP 27
        JP NZ,editorErrorWait
        RET
; Startup errors precede the editor UI and use the console's normal text.
editorStartupError:
        CALL disableInverse
        COLOR COLOR_BIOS
        CALL bios_printString
        LD HL,editorStartupWaitText
        CALL bios_printString
editorStartupErrorWait:
        CALL bios_getch
        CP 13
        JP NZ,editorStartupErrorWait
        RET
editorStartupWaitText: DB 13,10,'Press Enter to return.',13,10,0
editorCloseModal:
        CALL disableInverse
        LD HL,(editorModalCursor)
        LD (bios_vars.cursorY),HL
    IF EDIT_BANKED
        JP ebRestoreModal
    ELSE
        LD A,(hexActive)
        OR A
        JP NZ,hexRestoreUnderlay
        CALL clearTextViewport
        JP redrawPreservingCursor
    ENDIF
editorOpenFileModal:
        LD HL,(editorFilePrompt)
        EX DE,HL
        LD HL,editorFileDescriptor
editorOpenInputModal:
        PUSH HL
        EX DE,HL
        LD (editorInputTitle),HL
        POP HL
        CALL editorModalBegin
        LD HL,(editorInputTitle)
        CALL uiModalTitle
        LD A,7
        CALL uiModalBodyY
        LD H,36
        LD L,E
        LD (bios_vars.cursorY),HL
        CALL enableInverse
        ; Input occupies 240 pixels inside the SDK's centered 256px window.
        LD A,(FONT_ENGINE_HEADER+5)
        LD C,A
        LD A,240
        LD B,0
editorInputColumns:
        INC B
        SUB C
        JP NC,editorInputColumns
        DEC B
        LD A,B
        LD (editorInputWidth),A
        RET
editorErrorTitle: DB 'Error',0
editorModalOk: DB 'OK [Enter]',0
editorModalCancel: DB 'Cancel [Esc]',0
editorErrorDescriptor:
        UI_MODAL_BEGIN 20,1,1
        UI_MODAL_BUTTON editorModalOk,0,0,1
        UI_MODAL_END
editorInfoDescriptor:
        UI_MODAL_BUTTONS_BEGIN 1,1
        UI_MODAL_BUTTON editorModalOk,0,0,1
        UI_MODAL_END
editorFileDescriptor:
        UI_MODAL_BEGIN 12,1,2
        UI_MODAL_BUTTON editorModalOk,0,0,1
        UI_MODAL_BUTTON editorModalCancel,0,1,0
        UI_MODAL_END
editorModalCursor: DW 0
editorInputWidth: DB 0
editorInputTitle: DW 0

editorExitPrompt:
        LD HL,editorSaveDescriptor
        XOR A
        LD BC,UI_FOCUS_GRID*256
        CALL uiModalFocusInit
        LD HL,editorSaveDescriptor
        CALL editorModalBegin
        LD HL,editorSaveQuestion
        CALL uiModalTitle
editorSaveWait:
        CALL uiModalReadChoice
        JP NC,editorSaveChosen
        CP 27
        JP Z,editorCloseModal
        AND 0DFh
        CP 'S'
        JP Z,editorSaveFile
        CP 'D'
        JP Z,editorDiscardFile
        CP 'C'
        JP Z,editorCloseModal
        JP editorSaveWait
editorSaveChosen:
        OR A
        JP Z,editorSaveFile
        DEC A
        JP Z,editorDiscardFile
        JP editorCloseModal
editorDiscardFile:
        CALL editorCloseModal
        JP editorRunPendingAction
editorSaveFile:
        CALL editorCloseModal
        CALL editorWriteDocument
        LD A,(v_documentDirty)
        OR A
        RET NZ
        JP editorRunPendingAction
editorSaveQuestion: DB 'Save changes?',0
editorSaveLabel: DB 'Save',0
editorDiscardLabel: DB 'Discard',0
editorCancelLabel: DB 'Cancel',0
editorSaveDescriptor:
        UI_MODAL_BUTTONS_BEGIN 1,3
        UI_MODAL_BUTTON editorSaveLabel,0,0,1
        UI_MODAL_BUTTON editorDiscardLabel,0,1,0
        UI_MODAL_BUTTON editorCancelLabel,0,2,0
        UI_MODAL_END
