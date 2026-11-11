; Инициализатор не должен загрузить DOS-DLL поверх собственного кода.
    INCLUDE "../../source/include/mxos.inc"
IMAGE_FLAGS = IMAGE_FLAG_DRIVER
    ORG DOS_BANK_DRIVER_ADDR
entry:
    ld hl,modulePath
    ld a,MODULE_TARGET_DOS
    MXOS_CALL MXOS_DOS_MODULE_LOAD
    push af
    pop de
    ld hl,05202h
    ld b,0
    ld a,MEM_OP_PUT16
    call mx_memory
    ret
modulePath: DB 'B:',05Ch,'DOSMOD.DLL',0
    DS MAX_PATH-($-modulePath),0
