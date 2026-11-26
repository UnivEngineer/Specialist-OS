    INCLUDE "../../sdk/asm/mxos.inc"
    INCLUDE "../../sdk/asm/formats/font.inc"
    INCLUDE "../../sdk/asm/formats/font-init.inc"
    ORG FONT_INIT_ADDR
    jp initialize
    DB 'FIN1',1

;----------------------------------------------------------------------------
; MXOS
;
; Подготовить таблицы движка шрифта по протоколу FIN1 для Compact 6×8.
;
; вход:
;   a = FONT_INIT_IMPORT или FONT_INIT_GENERATE. hl = адрес FONT_INIT_REQUEST в MAIN.
;   Для импорта flat указывает на 256 глифов по 8 строк.
;
; выход:
;   IMPORT обновляет 162 глифа в промежуточном образе движка (коды
;   20h–7Fh, C0h–FFh, A3h и B3h). GENERATE не требует нижних таблиц.
;   При успехе a = 0, cf = 0. Для неизвестной операции a = DOS_ERR_ARGUMENT,
;   cf = 1. Остальные регистры могут меняться.
;----------------------------------------------------------------------------

initialize:
    or a
    jp z,importFlat
    cp FONT_INIT_GENERATE
    jp nz,bad
    xor a
    ret
bad:
    ld a,DOS_ERR_ARGUMENT
    scf
    ret
importFlat:
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    push de
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld (source),hl
    pop hl
    push hl
    ld de,42
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld de,FONT_ENGINE_BASE
    call bios_sub_hl_de
    pop de
    add hl,de
    ex de,hl ; DE=staged compact font, not its resident address.
    ld hl,codes
    ld c,codesEnd-codes
nextGlyph:
    ld a,(hl)
    inc hl
    push hl
    push bc
    ld l,a
    ld h,0
    add hl,hl
    add hl,hl
    add hl,hl
    push hl
    ld hl,(source)
    ld b,h
    ld c,l
    pop hl
    add hl,bc
    ld b,8
nextRow:
    ld a,(hl)
    and 63
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,nextRow
    pop bc
    pop hl
    dec c
    jp nz,nextGlyph
    xor a
    ret
source:         DW      0
codes:          INCBIN  "../../bin/fonts/compact-codes.bin"
codesEnd:
    ASSERT codesEnd-codes <= 255
    ASSERT $ <= FONT_INIT_END
