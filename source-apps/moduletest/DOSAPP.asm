; Тест одноразового инициализатора в банке DOS, без установки резидента.
    INCLUDE "../../source/include/mxos.inc"
IMAGE_FLAGS = IMAGE_FLAG_DRIVER
    ORG DOS_BANK_DRIVER_ADDR

;----------------------------------------------------------------------------
; MXOS
;
; Проверить работу одноразового инициализатора в странице DOS.
;
; вход:
;   Доступны консоль BIOS и MEM_OP_PUT16. Регистровых аргументов нет.
;
; выход:
;   После печати в MAIN по 8800h записывается 55AAh. Возвращается статус вызова
;   mx_memory.
;----------------------------------------------------------------------------

entry:
    ld hl,message
    MXOS_CALL MXOS_BIOS_PRINT_STRING
    ld hl,08800h
    ld de,055AAh
    ld b,0
    ld a,MEM_OP_PUT16
    call mx_memory
    ret
message: DB 'Native DOS initializer',13,10,0
