; Fixed KOI-7R tables are embedded. Flat-font import requires a generator.
    INCLUDE "../include/mxos.inc"
    INCLUDE "../include/font-init.inc"
    ORG FONT_INIT_ADDR
    jp initialize
    DB 'FIN1',1

;----------------------------------------------------------------------------
; MXOS
;
; Подготовить таблицы движка шрифта по протоколу FIN1.
;
; вход:
;   a = FONT_INIT_GENERATE. Таблицы KOI-7 встроены в образ, пакет запроса не читается.
;
; выход:
;   При генерации a = 0, cf = 0. Для любой другой операции a = DOS_ERR_UNSUPPORTED, cf
;   = 1. bc, de и hl сохранены.
;----------------------------------------------------------------------------

initialize:
    cp FONT_INIT_GENERATE
    jp z,ready
    ld a,DOS_ERR_UNSUPPORTED
    scf
    ret
ready:
    xor a
    ret
