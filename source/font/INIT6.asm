    INCLUDE "../../sdk/asm/mxos.inc"
    INCLUDE "../../sdk/asm/formats/font.inc"
    INCLUDE "../../sdk/asm/formats/font-init.inc"
    ORG FONT_INIT_ADDR
    jp initialize
    DB 'FIN1',1

;----------------------------------------------------------------------------
; MXOS
;
; Подготовить таблицы движка шрифта по протоколу FIN1 для 6×8.
;
; вход:
;   a = FONT_INIT_IMPORT или FONT_INIT_GENERATE. hl = адрес FONT_INIT_REQUEST в MAIN.
;   Для импорта flat указывает на 256 глифов по 8 строк.
;
; выход:
;   При IMPORT обновлены восемь страниц глифов в промежуточном образе
;   движка, при GENERATE подготовлены нижние плоскости Packed или Fast
;   (Rotate обходится без них). a = 0, cf = 0. Остальные регистры меняются.
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
    cp 2
    jp z,generateFull
    cp 1
    jp z,generatePacked
    xor a
    ret
generateFull:
    call prepareFull
    xor a
    ret
generatePacked:
    call prepareLower
    xor a
    ret
importFlat:
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
    and 63
    push bc
    ld b,a
    ld a,(mode)
    cp 1
    ld a,b
    jp nz,importStore
    rrca
    rrca
    and 0C0h
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
prepareSource:
    ld hl,(source)
    ret
    INCLUDE "prepare-packed.inc"
    INCLUDE "prepare-full.inc"
source:         DW 0
flatSource:     DW 0
lowerBase:      DW 0
mode:           DB 0
operation:      DB 0
    ASSERT $ <= packedP3
