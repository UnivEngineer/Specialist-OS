; Восьмибайтная таблица абсолютных старших байтов адреса блока.
; KOI-7: blockPages+3 = PAGED_FONT_BASE/256+4; KOI-8: +2.
; Вход ядра — только 20h..7Fh, C0h..FFh. Корпус нормализуется заранее.
    ALIGN 8,0
blockPages:
    DB PAGED_FONT_BASE/256,PAGED_FONT_BASE/256,PAGED_FONT_BASE/256+1,PAGED_FONT_BASE/256+2
    DB PAGED_FONT_BASE/256,PAGED_FONT_BASE/256,PAGED_FONT_BASE/256+3,PAGED_FONT_BASE/256+4
blockPointers: DS 8,0
    MACRO BLOCK_POINTER offset
    ld a,(paged.codes+offset)
    rlca
    rlca
    rlca
    ld c,a
    and 7
    or blockPages & 0FFh
    ld l,a
    ld h,blockPages >> 8
    ld h,(hl)
    ld a,c
    and 0F8h
    ld l,a
    ld (blockPointers+offset*2),hl
    ENDM
blockPreparePointers:
    BLOCK_POINTER 0
    BLOCK_POINTER 1
    BLOCK_POINTER 2
    BLOCK_POINTER 3
    ret
; Five blocks of 32 complete linear glyphs, built from the installed font.
blockBuild:
    ld de,PAGED_FONT_BASE
    ld c,32
blockBuildCode:
    ld h,FONT_ADDR >> 8
    ld l,c
    DUP 8
    ld a,(hl)
    ld (de),a
    inc h
    inc de
    EDUP
    inc c
    ret z
    ld a,c
    cp 128
    jp nz,blockBuildCode
    ld c,192
    jp blockBuildCode
; Raw must retain all 256 codes. Missing blocks use the independent full-font
; scalar kernel, four at a time; represented quads use the compact backend.
blockBatch:
    push af
    push bc
    push de
    push hl
    ld a,c
    ld (blockLeft),a
blockNext:
    push hl
    ld b,4
blockCheck:
    ld a,(hl)
    cp 32
    jp c,blockFallback
    cp 128
    jp c,blockValid
    cp 192
    jp c,blockFallback
blockValid:
    inc hl
    dec b
    jp nz,blockCheck
    pop hl
    ld c,4
blockRender:
    call batch6Raw
    ld bc,4
    add hl,bc
    jp blockConsumed
blockFallback:
    pop hl
    ld b,4
blockScalar:
    ld c,(hl)
    push bc
    push hl
    call scalar
    pop hl
    pop bc
    inc hl
    dec b
    jp nz,blockScalar
blockConsumed:
    ld a,(blockLeft)
    sub 4
    ld (blockLeft),a
    jp nz,blockNext
    pop hl
    pop de
    pop bc
    pop af
    ret
blockLeft: DB 0
