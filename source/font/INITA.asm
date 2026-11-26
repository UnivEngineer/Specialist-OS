    INCLUDE "../../sdk/asm/mxos.inc"
    INCLUDE "../../sdk/asm/formats/font.inc"
    INCLUDE "../../sdk/asm/formats/font-init.inc"
    ORG FONT_INIT_ADDR
    jp initialize
    DB 'FIN1',1

;----------------------------------------------------------------------------
; MXOS
;
; Подготовить таблицы движка шрифта по протоколу FIN1 для 8×8 и 4×8.
;
; вход:
;   a = FONT_INIT_IMPORT или FONT_INIT_GENERATE. hl = адрес FONT_INIT_REQUEST в MAIN.
;   Для импорта flat указывает на 256 глифов по 8 строк.
;
; выход:
;   При IMPORT обновлены данные промежуточного образа движка, при GENERATE
;   построены нижние таблицы для графических Packed/Fast 8×8 и Fast 4×8.
;   Для остальных профилей GENERATE не меняет таблицы. a = 0, cf = 0.
;   Остальные регистры могут меняться.
;----------------------------------------------------------------------------

initialize:
    ld (operation),a
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    push de
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ex de,hl
    ld (flatSource),hl
    ex de,hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld (lowerBase),hl
    ex de,hl
    pop hl
    push hl
    ld de,39
    add hl,de
    ld a,(hl)
    ld (mode),a
    inc hl
    inc hl
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld bc,FONT_ENGINE_BASE
    push de
    ld d,b
    ld e,c
    call bios_sub_hl_de
    pop de
    pop bc
    add hl,bc
    ld (source),hl
    ld a,(operation)
    or a
    jp z,importFlat
    ld a,(mode)
    cp FONT_FORMAT_GRAPH8_PACKED
    jp z,preparePacked8
    cp FONT_FORMAT_GRAPH8_FULL
    jp z,prepareFull8
    cp FONT_FORMAT_4_FAST
    jp z,prepareFast4
    xor a
    ret
importFlat:
    ld a,(mode)
    cp FONT_FORMAT_4_COMPACT
    jp z,importCompact4
    ; Transpose glyph-major c*8+y to row pages y*256+c. This conversion
    ; belongs to the initializer: FONT does not know engine bitmap layouts.
    ld hl,(source)
    ex de,hl
    ld hl,(flatSource)
    ld c,8
importRow:
    push hl
    ld b,0
importGlyph:
    ld a,(hl)
    push bc
    ld b,a
    ld a,(mode)
    cp FONT_FORMAT_4_PACKED
    jp z,importPacked4
    cp FONT_FORMAT_4_FAST
    ld a,b
    jp nz,importStore
    and 0Fh
    ld b,0
    jp importHigh4
importPacked4:
    ld a,b
    and 0Fh
    ld b,a
importHigh4:
    DUP 4
        rlca
    EDUP
    or b
importStore:
    ld (de),a
    ld bc,8
    add hl,bc
    pop bc
    inc e
    dec b
    jp nz,importGlyph
    inc d
    pop hl
    inc hl
    dec c
    jp nz,importRow
    xor a
    ret
; Строит восемь страниц младших полубайтов из верхней таблицы Fast 4×8.
prepareFast4:
    ld hl,(lowerBase)
    ex de,hl
    ld hl,(source)
    ld bc,FONT_BYTES
prepareFast4Byte:
    ld a,(hl)
    DUP 4
        rrca
    EDUP
    ld (de),a
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,prepareFast4Byte
    ret
; Плоский шрифт выровнен по странице. Собираем по восемь байтов на пару кодов.
importCompact4:
    ld hl,(source)
    ex de,hl
    ld hl,(flatSource)
    ld b,128
importCompactPair:
    push bc
    ld b,8
importCompactRow:
    ld a,(hl)
    and 0Fh
    DUP 4
        rlca
    EDUP
    ld c,a
    push hl
    ld a,l
    add a,8
    ld l,a
    ld a,(hl)
    pop hl
    and 0Fh
    or c
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,importCompactRow
    ld bc,8
    add hl,bc
    pop bc
    dec b
    jp nz,importCompactPair
    xor a
    ret
    INCLUDE "prepare-graph8.inc"
source:         DW 0
flatSource:     DW 0
lowerBase:      DW 0
mode:           DB 0
operation:      DB 0
    ASSERT $ <= FONT_INIT_END
