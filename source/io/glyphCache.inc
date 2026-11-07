; A=0: HL=начало, DE=исключающий конец, BC=ёмкость, A=эпоха, CF=0.
; A=1: HL=новый конец, DE=старый конец. Расширение сохраняет кэш.
; A=2: расширить до конца арены. A=3: после загрузки нового шрифта.
; Сужение консервативно инвалидирует индекс; будущий движок может отбрасывать
; только вытесненные записи. Верхние утилиты сохраняют прежний конец сами.
glyphCacheService:
    or a
    jp z,glyphCacheQuery
    dec a
    jp z,glyphCacheSet
    dec a
    jp z,glyphCacheFull
    dec a
    jp nz,glyphCacheBad
    ld hl,(FONT_SIZE_FIELD)
    ld de,FONT_ADDR
    add hl,de
    jp c,glyphCacheBad
    ld de,GLYPH_CACHE_END
    call bios_cmp_hl_de
    jp nc,glyphCacheBad
    ld (GLYPH_CACHE_START),hl
    ld hl,GLYPH_CACHE_END
    ld (GLYPH_CACHE_LIMIT),hl
    jp glyphCacheInvalidate
glyphCacheFull:
    ld hl,GLYPH_CACHE_END
glyphCacheSet:
    ld de,GLYPH_CACHE_END
    call bios_cmp_hl_de
    jp c,glyphCacheBelowEnd
    jp nz,glyphCacheBad
glyphCacheBelowEnd:
    push hl
    ld hl,(GLYPH_CACHE_START)
    ex de,hl
    pop hl
    call bios_cmp_hl_de
    jp c,glyphCacheBad
    push hl
    ld hl,(GLYPH_CACHE_LIMIT)
    ex de,hl
    pop hl
    call bios_cmp_hl_de
    ld (GLYPH_CACHE_LIMIT),hl
    jp nc,glyphCacheUnchanged
    ; Сужение: сохраняем DE старой границы, возвращаем CF=0.
glyphCacheInvalidate:
    ld hl,0
    ld (GLYPH_CACHE_ENTRIES),hl
    ld a,1
    ld (GLYPH_CACHE_INVALID),a
    ld hl,GLYPH_CACHE_EPOCH
    inc (hl)
glyphCacheUnchanged:
    xor a
    ret
glyphCacheBad:
    ld a,DOS_ERR_ARGUMENT
    scf
    ret
glyphCacheQuery:
    ld hl,(GLYPH_CACHE_LIMIT)
    push hl
    push hl
    ld hl,(GLYPH_CACHE_START)
    ex de,hl
    pop hl
    call bios_sub_hl_de
    ld b,h
    ld c,l
    pop de
    ld hl,(GLYPH_CACHE_START)
    ld a,(GLYPH_CACHE_EPOCH)
    or a
    ret
glyphCacheEnd:

; Холодная установка/Reset: очистка плотного блока рабочих переменных IO.
; AF/B/HL разрушены; интерфейс внутренних служебных данных не публичный.
ioInitialize:
    ld hl,io_vars
    ld b,IO_VARIABLES
    xor a
ioInitializeLoop:
    ld (hl),a
    inc hl
    dec b
    jp nz,ioInitializeLoop
    ret
ioInitializeEnd:
