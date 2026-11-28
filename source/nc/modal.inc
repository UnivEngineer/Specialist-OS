; NC owns snapshot/palette and keyboard policy; shared UI owns geometry.
ncModalSnapshot:
    push hl
    ld de,ncEditorLoadingModal+7
    call mon2_cmp_hl_de
    pop hl
    call nz,ncDialogOpen
    COLOR COLOR_DIALOG
    ret
ncModalOpen:
    ld a,(ncFontWidth)
    jp uiModalOpen
; The editor replaces the whole screen, so this notice needs no snapshot.
ncEditorLoading:
    ld hl,ncEditorLoadingModal
    call ncModalOpen
    ld hl,ncEditorLoadingText
    jp uiModalTitle
ncEditorLoadingText: DB 'Loading...',0
ncEditorLoadingModal:
    UI_MODAL_NOTICE

ncModalInputCursor:
    ; The editor's DE points to the initial text terminator. Positioning the
    ; modal must not replace its low byte with a screen baseline.
    push de
    ld a,7
    call uiModalBodyY
    ld h,36
    ld l,e
    pop de
    ret

; Catalog reads and sorting use the snapshot workspace as scratch. Open the
; warning only after both lists and the selected-file cursor have been drawn.
ncCatalogShowWarnings:
    xor a
    ld (ncCatalogWarnPending),a
    call ncCatalogWarnPanel
    ld a,(ncDamageMask)
    or a
    jp nz,ncCatalogWarningDamage
    ld a,1
    call ncCatalogWarnPanel
    ld a,(ncDamageMask)
    or a
    ret z
ncCatalogWarningDamage:
    scf
    ret
ncCatalogWarnPanel:
    ld e,a
    ld d,0
    ld hl,state.panelA_info
    add hl,de
    ld a,(hl)
    or a
    ret nz
    ld a,e
    call ncCatalogPanelOverflow
    cp 1
    ret nz
    ld (hl),2
    ld a,c
    call ncCatalogFormatWarning
    ; The selected-file highlight is visible in the saved pixels.
    ld a,(ncCommandCursorKept)
    push af
    ld a,1
    ld (ncCommandCursorKept),a
    ld hl,ncCatalogWarningModal
    call ncModalOpen
    ld hl,ncCatalogWarningTitle
    call uiModalTitle
    ld a,10
    call uiModalBodyY
    ld hl,ncCatalogWarningMemory
    call ncPrintCentered
    ld a,22
    call uiModalBodyY
    ld hl,vars.displayPath
    call ncPrintCentered
    IF NC_BANKED
    call ncBankFlush
    ENDIF
ncCatalogWarningRelease:
    call ncKeyScan
    inc a
    jp nz,ncCatalogWarningRelease
ncCatalogWarningKey:
    call uiModalGetch
    cp 13
    jp z,ncCatalogWarningClose
    cp 01Bh
    jp nz,ncCatalogWarningKey
ncCatalogWarningClose:
    call ncDialogClose
    pop af
    ld (ncCommandCursorKept),a
    ret

; A=slot. Format up to 30 characters so the line fits even at 8x8.
ncCatalogFormatWarning:
    call ncCatalogTotalAddress
    ld e,(hl)
    inc hl
    ld d,(hl)
    push de
    ld hl,ncCatalogDisplayedText
    ld de,vars.displayPath
    call copyPanelPath
    dec de
    push de
    ld a,(ncCatalogCapacity)
    ld e,a
    ld d,0
    ld b,0
    call ncFormatDecimal
    pop de
    call copyPanelPath
    dec de
    ld hl,ncCatalogRecordsText
    call copyPanelPath
    dec de
    pop hl
    push de
    ex de,hl
    ld b,0
    call ncFormatDecimal
    pop de
    jp copyPanelPath
ncCatalogWarnPending:       DB 0
ncCatalogWarningTitle:      DB 'Directory is too large',0
ncCatalogWarningMemory:     DB 'Not enough memory.',0
ncCatalogDisplayedText:     DB 'Displayed ',0
ncCatalogRecordsText:       DB ' records of ',0
ncCatalogWarningModal:
    UI_MODAL_BEGIN 26,1,1
    UI_MODAL_BUTTON ncErrorHint,0,0,1
    UI_MODAL_END
