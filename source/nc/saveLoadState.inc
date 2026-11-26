; Text state is an overlay on C:\DOS\NC.CFG. No private binary format or migration.
; Save only navigation/view fields, leaving the system color configuration alone.
    ASSERT 2*MAX_PATH+2*13+11*20 < 2048 ; Maximum two paths/leaves plus keys/numbers.
loadState:
    IF NC_BANKED
    ld hl,ncBankSnapshot
    ld de,RESIDENT_STATE_BYTES
    ld b,0
    ld a,RESIDENT_SHELL_STATE
    MXOS_CALL_REGS MXOS_DOS_RESIDENT
    jp c,ncStateColdFile
    call ncBankUnpackState
    jp ncStateValidate
ncStateColdFile:
    call ncBankStateFile
    jp c,ncStateValidate
    ELSE
    call ncRamStatePath
    ENDIF
    call ncConfigRead
    jp nc,ncStateValidate
    cp DOS_ERR_NOT_FOUND
    jp z,ncStateValidate
    ld a,1
    ld (ncStateDisabled),a
ncStateValidate:
    ; ActivePanel is range-checked by the parser. It must display files.
    xor a
    ld hl,state.panelA_info
    jp writeBytePanel

; Configuration key IDs 4..14 share the tokenizer with colors/default sorting.
ncStateConfig:
    call ncConfigToken
    jp c,ncStateEmptyValue
    ld a,(ncConfigKind)
    cp 5
    jp z,ncStateLeftPath
    cp 6
    jp z,ncStateRightPath
    cp 11
    jp z,ncStateLeftSelected
    cp 12
    jp z,ncStateRightSelected
    call ncStateReadByte
    ret c
    ld hl,ncStateByteFields
ncStateByteField:
    ld a,(ncConfigKind)
    cp (hl)
    jp z,ncStateByteFound
    inc hl
    inc hl
    inc hl
    jp ncStateByteField
ncStateByteFound:
    inc hl
    ld e,(hl)
    ld d,0
    inc hl
    ld a,(hl)
    cp c
    ret c
    ld hl,state
    add hl,de
    ld (hl),c
    ret
; Decimal byte, DE=token B=length -> C=value, CF=invalid/overflow.
ncStateReadByte:
    ld a,b
    cp 4
    jp nc,ncConfigNoToken
    ld c,0
ncStateByteDigit:
    ld a,(de)
    sub '0'
    cp 10
    jp nc,ncConfigNoToken
    ld l,a
    ld a,c
    cp 26
    jp nc,ncConfigNoToken
    cp 25
    jp nz,ncStateByteMultiply
    ld a,l
    cp 6
    jp nc,ncConfigNoToken
ncStateByteMultiply:
    ld a,c
    add a,a
    ld h,a
    add a,a
    add a,a
    add a,h
    add a,l
    ld c,a
    inc de
    dec b
    jp nz,ncStateByteDigit
    or a
    ret
ncStateLeftPath:
    xor a
    jp ncStatePathValue
ncStateRightPath:
    ld a,1
ncStatePathValue:
    ld c,a
    ld a,b
    cp MAX_PATH
    ret nc
    ld a,(state.activePanel)
    push af
    ld a,c
    ld (state.activePanel),a
    ex de,hl
    ; Resolve an absolute path afresh; no on-disk cluster IDs become stale.
    ld a,CLI_OP_CHDIR
    MXOS_CALL_REGS MXOS_DOS_CLI
    jp c,ncStatePathDone
    ld e,0
    MXOS_CALL_REGS MXOS_DOS_FILE_GET_SET_DRIVE
    ld hl,state.panelA_drive
    call writeBytePanel
    call savePanelDirectory
    xor a
    ld hl,vars.pathValidA
    call writeBytePanel
ncStatePathDone:
    pop af
    ld (state.activePanel),a
    ret
ncStateLeftSelected:
    ld hl,vars.savedSelectedA
    jp ncStateSelectedValue
ncStateRightSelected:
    ld hl,vars.savedSelectedB
ncStateSelectedValue:
    ld a,b
    or a
    jp z,ncStateEmptyValue
    cp 13
    ret nc
ncStateSelectedByte:
    ld a,(de)
    ld (hl),a
    inc de
    inc hl
    or a
    jp nz,ncStateSelectedByte
    ld a,1
    jp ncStateSelectedFlag
ncStateEmptyValue:
    cp 1 ; Unterminated quoted token is invalid, not an explicit empty value.
    ret z
    ld a,(ncConfigKind)
    cp 11
    jp z,ncStateEmptyLeft
    cp 12
    ret nz
    ld hl,vars.savedSelectedB
    jp ncStateEmptySelected
ncStateEmptyLeft:
    ld hl,vars.savedSelectedA
ncStateEmptySelected:
    xor a
    ld (hl),a
ncStateSelectedFlag:
    ld hl,vars.restoreSelectedA
    ld b,a
    ld a,(ncConfigKind)
    cp 11
    jp z,ncStateSelectedSet
    inc hl
ncStateSelectedSet:
    ld (hl),b
    ret
; Restore once after the fresh sorted index is available, including hidden F3.
ncStateSelect:
    ld hl,vars.restoreSelectedA
    call readBytePanel
    or a
    ret z
    xor a
    call writeBytePanel
    call ncStateSelectedBase
    xor a
    jp ncSelectLeaf
ncStateSelectedBase:
    ld hl,vars.savedSelectedA
    ld a,(state.activePanel)
    or a
    ret z
    ld hl,vars.savedSelectedB
    ret

saveState:
    IF NC_BANKED
    call ncBankPackState
    ld hl,ncBankSnapshot
    ld de,RESIDENT_STATE_BYTES
    ld b,1
    ld a,RESIDENT_SHELL_STATE
    MXOS_CALL_REGS MXOS_DOS_RESIDENT
    call ncBankStateFile
    ret c
    ENDIF
    ld a,(ncStateDisabled)
    or a
    ret nz
    ld e,0
    MXOS_CALL_REGS MXOS_DOS_FILE_GET_SET_DRIVE
    ld (ncStateDrive),a
    ld a,(state.activePanel)
    ld (ncStateActive),a
    ; All index preparation precedes serialization: radix uses the same scratch.
    xor a
    call ncStateCapture
    ld a,1
    call ncStateCapture
    ld a,(ncStateActive)
    ld (state.activePanel),a
    ld hl,state.panelA_drive
    call readBytePanel
    call loadFiles
    ld de,NC_SORT_KEYS
    ld hl,ncKeyActive
    call ncStateWriteKey
    ld a,(ncStateActive)
    call ncStateWriteNumber
    xor a
    call ncStateWritePanel
    ld a,1
    call ncStateWritePanel
    ; Restore the active list pointer without running a second radix pass.
    ld a,(ncStateActive)
    ld (state.activePanel),a
    push de
    call fileIndexBase
    push hl
    ld hl,state.panelA_firstFile
    call readBytePanel
    pop hl
    ld e,a
    ld d,0
    add hl,de
    ld (vars.fileListPtr),hl
    pop hl
    ld de,NC_SORT_KEYS
    call bios_sub_hl_de
    ld (ncStateIO+FILE_PATH_IO.count),hl
    IF NC_BANKED
    ld hl,vars.stateFile
    ELSE
    call ncRamStatePath
    ENDIF
    call ncConfigSetPath
    ld hl,NC_SORT_KEYS
    ld (ncStateIO+FILE_PATH_IO.buffer),hl
    ld hl,ncStateIO
    call fileWriteAll
    jp c,ncStateDisable
    ld hl,ncStateIO+FILE_PATH_IO.descriptor
    ld bc,0201h ; Hide the generated state from ordinary directory listings.
    MXOS_CALL_REGS MXOS_DOS_FILE_GET_SET_ATTR
    jp c,ncStateDisable
    jp ncStateFinish
ncStateDisable:
    ld a,1
    ld (ncStateDisabled),a
ncStateFinish:
    ld a,(ncStateDrive)
    MXOS_CALL_REGS MXOS_DOS_FILE_SELECT_DRIVE
    ret
ncStateCapture:
    ld (state.activePanel),a
    ld hl,state.panelA_drive
    call readBytePanel
    call loadFiles
    call getSelectedFile
    ld a,(hl)
    cp 0FFh
    push af
    push hl
    call ncStateSelectedBase
    ex de,hl
    pop hl
    pop af
    jp z,ncStateCaptureEmpty
    jp compactName
ncStateCaptureEmpty:
    xor a
    ld (de),a
    ret
; A=panel, DE=serialized output. Key labels are shared with the parser table.
ncStateWritePanel:
    ld (state.activePanel),a
    ld hl,ncStateLeftKeys
    or a
    jp z,ncStatePanelKeys
    ld hl,ncStateRightKeys
ncStatePanelKeys:
    ld (ncStateKeys),hl
    call ncStateNextKey
    push de
    ld hl,state.panelA_drive
    call readBytePanel
    MXOS_CALL_REGS MXOS_DOS_FILE_SELECT_DRIVE
    call restorePanelDirectory
    call getPanelPath
    pop de
    jp nc,ncStatePathReady
    ; An unavailable saved drive is written as its root; next load can reject it.
    push de
    ld hl,state.panelA_drive
    call readBytePanel
    add a,'A'
    ld hl,vars.pathArg
    ld (hl),a
    inc hl
    ld (hl),':'
    inc hl
    ld (hl),05Ch
    inc hl
    ld (hl),0
    ld hl,vars.pathArg
    pop de
ncStatePathReady:
    call ncStateWriteString
    call ncStateWriteNewline
    call ncStateNextKey
    ld hl,state.panelA_sort
    call readBytePanel
    call ncStateWriteNumber
    call ncStateNextKey
    ld hl,state.panelA_info
    call readBytePanel
    call ncStateWriteNumber
    call ncStateNextKey
    call ncStateSelectedBase
    call ncStateWriteSelected
    call ncStateWriteNewline
    call ncStateNextKey
    ld hl,state.panelA_firstFile
    call readBytePanel
    jp ncStateWriteNumber
ncStateNextKey:
    ld hl,(ncStateKeys)
    ld c,(hl)
    inc hl
    ld b,(hl)
    inc hl
    ld (ncStateKeys),hl
    ld h,b
    ld l,c
ncStateWriteKey:
    call ncStateWriteString
    ld a,' '
    ld (de),a
    inc de
    ret
ncStateWriteString:
    ld a,(hl)
    or a
    ret z
    ld (de),a
    inc hl
    inc de
    jp ncStateWriteString
ncStateWriteSelected:
    ld a,(hl)
    cp '#'
    jp nz,ncStateWriteString
    ld a,'"'
    ld (de),a
    inc de
    call ncStateWriteString
    ld a,'"'
    ld (de),a
    inc de
    ret
; A=0..255 -> decimal + CRLF. DE advances, AF/BC/HL destroyed.
ncStateWriteNumber:
    ld b,a
    ld c,'0'
    ld h,0
ncStateHundreds:
    sub 100
    jp c,ncStateHundredsDone
    inc c
    jp ncStateHundreds
ncStateHundredsDone:
    add a,100
    ld b,a
    ld a,c
    cp '0'
    jp z,ncStateTensBegin
    ld (de),a
    inc de
    inc h
ncStateTensBegin:
    ld a,b
    ld c,'0'
ncStateTens:
    sub 10
    jp c,ncStateTensDone
    inc c
    jp ncStateTens
ncStateTensDone:
    add a,10
    ld b,a
    ld a,c
    cp '0'
    jp nz,ncStateWriteTens
    ld a,h
    or a
    jp z,ncStateOnes
    ld a,c
ncStateWriteTens:
    ld (de),a
    inc de
ncStateOnes:
    ld a,b
    add a,'0'
    ld (de),a
    inc de

;----------------------------------------------------------------------------
; MXOS
;
; Дописать перевод строки DOS в буфер конфигурации NC.
;
; вход:
;   de = адрес двух доступных для записи байтов.
;
; выход:
;   Записаны CR и LF, de указывает за LF, a = 10. bc, hl и флаги сохранены.
;----------------------------------------------------------------------------

ncStateWriteNewline:
    ld a,13
    ld (de),a
    inc de
    ld a,10
    ld (de),a
    inc de
    ret
ncStateByteFields:
    DB 4,state.activePanel-state,1
    DB 7,state.panelA_sort-state,2,8,state.panelB_sort-state,2
    DB 9,state.panelA_info-state,1,10,state.panelB_info-state,1
    DB 13,state.panelA_firstFile-state,255,14,state.panelB_firstFile-state,255
ncStateLeftKeys:
    DW ncKeyLeftPath,ncKeyLeftSort,ncKeyLeftInfo,ncKeyLeftSelected,ncKeyLeftFirst
ncStateRightKeys:
    DW ncKeyRightPath,ncKeyRightSort,ncKeyRightInfo,ncKeyRightSelected,ncKeyRightFirst
ncStateDisabled:    DB 0
ncStateDrive:       DB 0
ncStateActive:      DB 0
ncStateKeys:        DW 0
ncStateIO:
    DW vars.pathArg,NC_SORT_KEYS,2047,0
    DS FILE_DESCRIPTOR,0
 ; Return HL = NC state path for the current RAM disk. AF/BC/DE are scratch.
ncRamStatePath:
    IF !NC_BANKED
    ld hl,vars.stateFile
    ld a,(hl)
    or a
    ret nz
    ld c,MEM_RAMDISK_STATUS
    ld a,MEM_OP_RAMDISK
    MXOS_CALL_REGS MXOS_DOS_MEMORY
    ld a,e
    add a,'A'
    ld (ncStatePath),a
    ld hl,ncStatePath
    ret
ncStatePath: DB 'B:',05Ch,'NC-STATE.CFG',0
    ELSE
;----------------------------------------------------------------------------
; MXOS
; Выбрать дисковую копию панелей банкового NC.
; вход: vars.stateFile содержит абсолютный путь либо пустую строку.
; выход: HL=путь, CF=0 для внешнего носителя, CF=1 при отсутствии пути или
; выборе RAM-диска. AF, BC, DE рабочие.
;----------------------------------------------------------------------------
ncBankStateFile:
    ld c,MEM_RAMDISK_STATUS
    ld a,MEM_OP_RAMDISK
    MXOS_CALL_REGS MXOS_DOS_MEMORY
    ret c
    ld hl,vars.stateFile
    ld a,(hl)
    sub 'A'
    ret c
    cp e
    scf
    ret z
    or a
    ret

;----------------------------------------------------------------------------
; MXOS
; Перенести параметры панелей и выбранные имена между EXEC и снимком DOS.
; вход: при упаковке обе панели подготовлены, при распаковке снимок прочитан
; в ncBankSnapshot. Первые 9 байтов содержат активную панель, каталоги,
; диски и прокрутку; следующие 4 — режимы, последние 22 — два имени FAT.
; выход: при упаковке активная панель и её список восстановлены. При
; распаковке выделение применяется после чтения каталога. Все регистры рабочие.
;----------------------------------------------------------------------------
ncBankPackState:
    ld hl,state.activePanel
    ld de,state.panelA_curFile
    ld bc,ncBankSnapshot
    call bios_memcpy_bc_hl
    ld hl,state.panelA_sort
    ld de,stateEnd
    call bios_memcpy_bc_hl
    xor a
    ld bc,ncBankSnapshot+13
    call ncBankPackLeaf
    ld a,1
    ld bc,ncBankSnapshot+24
    call ncBankPackLeaf
    ld a,(ncBankSnapshot)
    ld (state.activePanel),a
    ld hl,state.panelA_drive
    call readBytePanel
    jp loadFiles
ncBankPackLeaf:
    push bc
    ld (state.activePanel),a
    ld hl,state.panelA_drive
    call readBytePanel
    call loadFiles
    call getSelectedFile
    pop bc
    ld a,(hl)
    cp 0FFh
    jp nz,ncBankPackName
    xor a
    ld (bc),a
    ret
ncBankPackName:
    push hl
    ld de,11
    add hl,de
    ex de,hl
    pop hl
    jp bios_memcpy_bc_hl
ncBankUnpackState:
    ld hl,ncBankSnapshot
    ld de,ncBankSnapshot+9
    ld bc,state.activePanel
    call bios_memcpy_bc_hl
    ld de,ncBankSnapshot+13
    ld bc,state.panelA_sort
    call bios_memcpy_bc_hl
    ld hl,ncBankSnapshot+13
    ld de,vars.savedSelectedA
    call ncBankUnpackLeaf
    ld (vars.restoreSelectedA),a
    ld hl,ncBankSnapshot+24
    ld de,vars.savedSelectedB
    call ncBankUnpackLeaf
    ld (vars.restoreSelectedB),a
    ret
ncBankUnpackLeaf:
    ld a,(hl)
    or a
    ret z
    call compactName
    ld a,1
    ret
ncBankSnapshot: DS RESIDENT_STATE_BYTES,0
    ASSERT state.panelA_curFile-state.activePanel == 9
    ASSERT stateEnd-state.panelA_sort == 4
    ASSERT 13+2*11 == RESIDENT_STATE_BYTES
    ENDIF
