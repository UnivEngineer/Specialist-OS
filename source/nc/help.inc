ncHelp:
    IF NC_BANKED
    ; This client passes no background buffer to UI_GFX.
    call ncDamageBoth
    ENDIF
    ld hl,ncHelpPath
    call uiShowHelp
    jp nc,ncHelpWaitRelease
    ld hl,ncErrorModal
    call ncModalOpen
    ld hl,ncHelpError
    call uiModalBodyText
    call uiModalGetch
ncHelpWaitRelease:
    ; Keep an autorepeat of the closing key inside the modal.
    call bios_keyScan
    inc a
    jp nz,ncHelpWaitRelease
    jp ncRefreshDirty
    IF !NC_BANKED
ncHelpColor:
    COLOR COLOR_DIALOG
    ret
    ENDIF
    DB 0 ; This client redraws its background.
ncHelpPath:     DB 'C:',05Ch,'DOS',05Ch,'NC.HLP',0
ncHelpError:    DB 'Cannot read C:/DOS/NC.HLP',0
