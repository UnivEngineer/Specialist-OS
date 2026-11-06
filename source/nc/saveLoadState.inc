; Состояние принадлежит NC. Пока оболочка работает в main, сохраняем его
; в B:\NCSTATE.TMP. Ошибки и нехватка места отключают сохранение до нового NC.
; Выбранный диск сохраняется, CWD не меняется файловым API.
saveState:
    ld a,(ncStateDisabled)
    or a
    ret nz
    call ncStateBegin
    ld hl,state
    ld (ncStateIO+FILE_PATH_IO.buffer),hl
    ld hl,ncStateIO
    call bios_fileSavePath
    jp c,ncStateDisable
    ; Служебный файл не попадает в обычные списки NC/DIR.
    ld hl,ncStateIO+FILE_PATH_IO.descriptor
    ld bc,0201h
    MXCALL MX_DOS_FILEGETSETATTR
    jp c,ncStateDisable
    jp ncStateFinish
loadState:
    call ncStateBegin
    ld hl,ncSavedState
    ld (ncStateIO+FILE_PATH_IO.buffer),hl
    ld hl,ncStateIO
    call bios_fileLoadPath
    jp nc,ncStateValidate
    ; Отсутствующий файл при первом запуске не отключает сохранение.
    MXCALL MX_DOS_GETERROR
    cp DOS_ERR_NOT_FOUND
    jp nz,ncStateDisable
    jp ncStateInvalid
ncStateValidate:
    ld hl,(ncStateIO+FILE_PATH_IO.actual)
    ld de,stateEnd-state
    call bios_cmp_hl_de
    jp nz,ncStateInvalid
    ld a,(ncSavedState)
    cp 05Bh
    jp nz,ncStateInvalid
    ld hl,ncSavedState
    ld de,ncSavedState+stateEnd-state
    ld bc,state
    call bios_memcpy_bc_hl
    xor a
    jp ncStateFinish
ncStateDisable:
    ld a,1
    ld (ncStateDisabled),a
ncStateInvalid:
    ld a,1
    or a
ncStateFinish:
    push af
    ld a,(ncStateDrive)
    MXCALL MX_DOS_FILESELECTDRIVE
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
