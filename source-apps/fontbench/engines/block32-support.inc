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
blockBuildFont:
    ; Линейный FONT.FNT в стенде: пять исходных страниц по 32 глифа.
    ld hl,02900h
    ld de,PAGED_FONT_BASE
    ld bc,0300h
    call bench_copy
    ld hl,02E00h
    ld de,PAGED_FONT_BASE+0300h
    ld bc,0200h
    jp bench_copy

blockNormalizeCorpus:
    ld hl,textCorpus
    ld bc,FONT_CORPUS_SIZE
blockNormalizeByte:
    ld a,(hl)
    cp 080h
    jp c,blockNormalizeNext
    cp 0C0h
    jp nc,blockNormalizeNext
    ld (hl),32
blockNormalizeNext:
    inc hl
    dec bc
    ld a,b
    or c
    jp nz,blockNormalizeByte
    ret
