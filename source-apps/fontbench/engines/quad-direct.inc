; Четверки без кеша. Четыре указателя читаются POP HL из рабочей памяти;
; экран в DE, B/C удерживают части текущей строки. SP сохранён на четверку.
; Пока SP указывает рабочие указатели, нет CALL/PUSH/RET, только POP данных.
; Все части глифов переходят в соседний экранный байт, а не отбрасываются.
quadDirectGroup:
    ld a,(quadUseFull)
    cp 3
    jp nz,quadDirectPointers
    ld hl,(quadHalfFont)
    ld a,h
    or l
    jp z,quadError
quadDirectPointers:
    ld hl,(quadText)
    ex de,hl
    ld a,(de)
    inc de
    ld c,a
    ld b,0
    ld hl,(renderFont8)
    add hl,bc
    add hl,hl
    add hl,hl
    add hl,hl
    ld (quadP0),hl
    ld a,(de)
    inc de
    ld c,a
    ld a,(quadUseFull)
    cp 3
    ld hl,(renderFont8)
    jp nz,quadDirectSecond
    ld hl,(quadHalfFont)
quadDirectSecond:
    add hl,bc
    add hl,hl
    add hl,hl
    add hl,hl
    ld (quadP6),hl
    ld a,(de)
    inc de
    ld c,a
    ld hl,(renderFont8)
    add hl,bc
    add hl,hl
    add hl,hl
    add hl,hl
    ld (quadP4),hl
    ld a,(de)
    inc de
    ld c,a
    ld hl,(renderFont8)
    add hl,bc
    add hl,hl
    add hl,hl
    add hl,hl
    ld (quadRaw),hl
    ex de,hl
    ld (quadText),hl
    ld hl,(bios_vars.cursorY)
    ld a,h
    rrca
    rrca
    and 03Fh
    add a,090h
    ld d,a
    ld a,l
    sub 7
    ld e,a
    ld hl,0
    add hl,sp
    ld (quadDirectRestore+1),hl
    ld hl,quadDirectPixels
    ld a,(quadUseFull)
    cp 3
    jp nz,quadDirectSelect
    ld hl,quadHalfPixels
quadDirectSelect:
    ld (quadDirectLoop+1),hl
    ld a,e
    add a,8
    ld (quadDirectEndY+1),a
quadDirectRepeat:
quadDirectLoop:
    jp quadDirectPixels

    MACRO QUAD_DIRECT_PIXELS half
    ; Четыре строки развёрнуты; две итерации сохраняют компактность кода.
    DUP 4
    ld sp,quadP0
    pop hl
    ld a,(hl)
    inc l
    ld (quadP0),hl
    add a,a
    add a,a
    ld b,a
    pop hl
    ld a,(hl)
    inc l
    ld (quadP6),hl
    IF !half
    rrca
    rrca
    rrca
    rrca
    ENDIF
    ld c,a
    and 3
    or b
    ld (de),a
    inc d
    ; Повторно используем тот же поворот b, без четырех новых RLCA.
    ld a,c
    and 0F0h
    ld b,a
    pop hl
    ld a,(hl)
    inc l
    ld (quadP4),hl
    rrca
    rrca
    ld c,a
    and 00Fh
    or b
    ld (de),a
    inc d
    ; Тот же поворот c даёт вытолкнутую часть третьего экранного байта.
    ld a,c
    and 0C0h
    ld b,a
    pop hl
    ld a,(hl)
    inc l
    ld (quadRaw),hl
    or b
    ld (de),a
    dec d
    dec d
    inc e
    EDUP
    ENDM
quadDirectPixels:
    QUAD_DIRECT_PIXELS 0
quadDirectPixelsEnd:
    jp quadDirectRowEnd
quadHalfPixels:
    QUAD_DIRECT_PIXELS 1
quadHalfPixelsEnd:
quadDirectRowEnd:
    ld a,e
quadDirectEndY:
    cp 0
    jp nz,quadDirectRepeat
quadDirectRestore:
    ld sp,0
    jp quadPixelsEnd

; Единственная неизменная таблица поворота на 4: 256*8, без lookup и FIFO.
; Подготовка выполняется отдельно от измерения; арена BIOS должна вмещать её.
quadPrepareHalf:
    call cacheDiscard
    ld hl,0
    ld (quadHalfFont),hl
    xor a
    IFDEF FONT_ENGINE_DLL
    ld hl,03000h
    ld bc,0800h
    ELSE
    MXOS_CALL MXOS_BIOS_GLYPH_CACHE
    ret c
    ENDIF
    ld a,l
    and 7
    jp nz,quadHalfUnavailable
    ld a,b
    cp 8
    jp nc,quadHalfSpace
    ; Полный FONT.FNT оставляет в верхней арене меньше 2 КБ.
    ; Half, как и кеш стенда, использует собственный main-резерв.
    IFDEF FONT_ENGINE_DLL
    ld hl,03000h
    ELSE
    ld hl,07E00h
    ENDIF
quadHalfSpace:
    ex de,hl
    ; Адрес знакогенератора хранится в единицах восьми байт, как charGen.
    ld h,d
    ld l,e
    DUP 3
    or a
    ld a,h
    rra
    ld h,a
    ld a,l
    rra
    ld l,a
    EDUP
    ld (quadHalfFont),hl
    ld hl,(renderFont8)
    add hl,hl
    add hl,hl
    add hl,hl
    ld bc,2048
quadHalfBuild:
    ld a,(hl)
    rrca
    rrca
    rrca
    rrca
    ld (de),a
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,quadHalfBuild
    or a
    ret
quadHalfUnavailable:
    scf
    ret
quadHalfFont: DW 0
