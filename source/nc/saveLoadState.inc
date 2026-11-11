; Состояние принадлежит NC. Пока оболочка работает в main, сохраняем его
; в B:\NCSTATE.TMP. Ошибки и нехватка места отключают сохранение до нового NC.
; Вызывается перед запуском внешней программы и выходом из NC.
; Выбранный диск сохраняется, CWD не меняется файловым API.
saveState:
    ld a,(ncStateDisabled)
    or a
    ret nz
    call ncStateBegin
    ld hl,state
    ld (ncStateIO+FILE_PATH_IO.buffer),hl
    ld hl,ncStateIO
    MXOS_CALL MXOS_DOS_FILE_SAVE_PATH
    jp c,ncStateDisable
    ; Служебный файл не попадает в обычные списки NC/DIR.
    ld hl,ncStateIO+FILE_PATH_IO.descriptor
    ld bc,0201h
    MXOS_CALL MXOS_DOS_FILE_GET_SET_ATTR
    jp c,ncStateDisable
    jp ncStateFinish
loadState:
    ld a,2
    ld (ncSavedState+state.panelA_sort-state),a
    ld (ncSavedState+state.panelB_sort-state),a
    call ncStateBegin
    ld hl,ncSavedState
    ld (ncStateIO+FILE_PATH_IO.buffer),hl
    ld hl,ncStateIO
    MXOS_CALL MXOS_DOS_FILE_LOAD_PATH
    jp nc,ncStateValidate
    ; Отсутствующий файл при первом запуске не отключает сохранение.
    MXOS_CALL MXOS_DOS_GET_ERROR
    cp DOS_ERR_NOT_FOUND
    jp nz,ncStateDisable
    jp ncStateInvalid
ncStateValidate:
    ld hl,(ncStateIO+FILE_PATH_IO.actual)
    ld de,stateEnd-state
    call bios_cmp_hl_de
    jp z,ncStateVersionReady
    ld de,state.panelA_sort-state
    call bios_cmp_hl_de
    jp nz,ncStateInvalid
ncStateVersionReady:
    ld a,(ncSavedState)
    cp 05Bh
    jp nz,ncStateInvalid
    ld hl,ncSavedState
    ld de,ncSavedState+stateEnd-state
    ld bc,state
    call bios_memcpy_bc_hl
    ld hl,state.panelA_sort
    call ncStateSortMode
    inc hl
    call ncStateSortMode
    xor a
    jp ncStateFinish
; Неизвестное значение режима не влияет на остальные сохранённые поля.
ncStateSortMode:
    ld a,(hl)
    cp 3
    ret c
    ld (hl),2
    ret
ncStateDisable:
    ld a,1
    ld (ncStateDisabled),a
ncStateInvalid:
    ld a,1
    or a
ncStateFinish:
    push af
    ld a,(ncStateDrive)
    MXOS_CALL MXOS_DOS_FILE_SELECT_DRIVE
    pop af
    ret
ncStateBegin:
    ld e,0
    call bios_fileGetSetDrive
    ld (ncStateDrive),a
    ld hl,stateEnd-state
    ld (ncStateIO+FILE_PATH_IO.count),hl
    ret
ncStateDisabled: DB 0
ncStateDrive: DB 0
ncSavedState: DS stateEnd-state,0
ncStateIO:
    DW ncStatePath,ncSavedState,stateEnd-state,0
    DS FILE_DESCRIPTOR,0
ncStatePath:
    DB 'B:',05Ch,'NCSTATE.TMP',0
    DS MAX_PATH-($-ncStatePath),0
