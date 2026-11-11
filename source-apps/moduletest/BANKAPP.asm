; Нативное банковое приложение: локальные аргументы DE, завершение через RET.
    INCLUDE "../../source/include/mxos.inc"
IMAGE_FLAGS = IMAGE_FLAG_BANK
    ORG 00100h
entry:
    ld a,(de)
    and 0DFh
    cp 'T'
    jp nz,failed
    ex de,hl
    ld (arguments),hl
    call mx_memInfo
    or a
    jp z,failed
    cp 1
    jp z,failed
    ld a,c
    cp MEM_BANK_EXEC
    jp nz,failed
    ld hl,message
    MXOS_CALL MXOS_BIOS_PRINT_STRING
    ld hl,(arguments)
    MXOS_CALL MXOS_BIOS_PRINT_STRING
    ld b,MEM_ALLOC_ANY
    ld hl,128
    ld a,MEM_OP_ALLOC
    MXOS_CALL MXOS_DOS_MEMORY
    ret c
    ld de,055AAh
    jp result
failed:
    ld de,0DEADh
result:
    ld hl,05200h
    ld b,0
    ld a,MEM_OP_PUT16
    call mx_memory
    ret
message: DB 'Native bank application: ',0
arguments: DW 0
