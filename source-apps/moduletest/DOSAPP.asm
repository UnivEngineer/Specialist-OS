; Тест одноразового инициализатора в банке DOS, без установки резидента.
    INCLUDE "../../source/include/mxos.inc"
IMAGE_FLAGS = IMAGE_FLAG_DRIVER
    ORG DOS_BANK_DRIVER_ADDR
entry:
    ld hl,message
    MXOS_CALL MXOS_BIOS_PRINT_STRING
    ld hl,05200h
    ld de,055AAh
    ld b,0
    ld a,MEM_OP_PUT16
    call mx_memory
    ret
message: DB 'Native DOS initializer',13,10,0
