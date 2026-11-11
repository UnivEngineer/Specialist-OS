; Выровненные четверки 6x8 прямо в экран: нет временной битмапы и чтений VRAM.
; HL=печатные KOI-8 байты, C=0..48, длина кратна 4, cursorX кратен 8 пикселям.
; Полная таблица и кеш используют один пиксельный код. BC/DE/HL и A сохранены;
; флаги разрушены, CF=1 при неверных параметрах/недостаточной емкости кеша.
quadRender:
    push af
    xor a
    jp quadEntry
quadFullRender:
    push af
    ld a,1
    jp quadEntry
quadDynamicRender:
    push af
    ld a,2
    jp quadEntry
quadHalfRender:
    push af
    ld a,3
    jp quadEntry
quadPackedRender:
    push af
    ld a,4
quadEntry:
    ld (quadUseFull),a
    push bc
    push de
    push hl
    ld (quadText),hl
    ld a,c
    or a
    jp z,quadSuccess
    cp 49
    jp nc,quadError
    and 3
    jp nz,quadError
    ld a,c
    ld (quadRemaining),a
    ld hl,(bios_vars.cursorY)
    ld a,h
    and 3
    jp nz,quadError
    ld a,l
    cp 7
    jp c,quadError
    ld a,c
    add a,a
    add a,c
    add a,h
    jp c,quadError
    cp 193
    jp nc,quadError
    ld a,(quadUseFull)
    or a
    jp nz,quadGroup
    ld a,(GLYPH_CACHE_INVALID)
    or a
    call nz,cachePrepare
    ld a,(cacheSlots)
    cp 3
    jp c,quadError
    ld hl,cacheChooseVictim
    ld (cacheVictimGuard+1),hl
quadGroup:
    ld a,(quadUseFull)
    cp 4
    jp z,quadPackedGroup
    cp 2
    jp nc,quadDirectGroup
    xor a
    ld (cacheLifeRecent),a
    ld (cacheLifeOlder),a
    ld hl,(quadText)
    ex de,hl
    ld a,(de)
    inc de
    ld c,a
    ld b,0
    call quadLookup
    ld (quadP0),hl
    ld a,(de)
    inc de
    ld c,a
    ld b,2
    call quadLookup
    ld (quadP6),hl
    ld a,(de)
    inc de
    ld c,a
    ld b,1
    call quadLookup
    ld (quadP4),hl
    ld a,(de)
    inc de
    ld c,a
    ld b,0
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
    ld b,a
    ld a,l
    sub 7
    ld c,a
    ; Экран в BC. Столбец пишется целиком, источники остаются в DE/HL.
    ; Между экранными столбцами только INC B, то есть +100h.
quadPixels:
    ld hl,(quadP0)
    ex de,hl
    ld hl,(quadP6)
    DUP 7
    ld a,(de)
    or (hl)
    ld (bc),a
    inc e
    inc l
    inc l
    inc c
    EDUP
    ld a,(de)
    or (hl)
    ld (bc),a
    inc b
    ld a,c
    sub 7
    ld c,a
    ld hl,(quadP6)
    inc l
    ex de,hl
    ld hl,(quadP4)
    DUP 7
    ld a,(de)
    or (hl)
    ld (bc),a
    inc e
    inc e
    inc l
    inc l
    inc c
    EDUP
    ld a,(de)
    or (hl)
    ld (bc),a
    inc b
    ld a,c
    sub 7
    ld c,a
    ld hl,(quadP4)
    inc l
    ex de,hl
    ld hl,(quadRaw)
    DUP 7
    ld a,(de)
    or (hl)
    ld (bc),a
    inc e
    inc e
    inc l
    inc c
    EDUP
    ld a,(de)
    or (hl)
    ld (bc),a
quadPixelsEnd:
    ld a,(bios_vars.cursorX)
    add a,12
    ld (bios_vars.cursorX),a
    ld hl,quadRemaining
    ld a,(hl)
    sub 4
    ld (hl),a
    jp nz,quadGroup
quadSuccess:
    ld hl,cacheVictimReady
    ld (cacheVictimGuard+1),hl
    pop hl
    pop de
    pop bc
    pop af
    or a
    ret
quadError:
    pop hl
    pop de
    pop bc
    pop af
    scf
    ret

; B=номер страницы фазы 0/4/6, C=код, HL=источник битмапы на выходе.
; Данные сразу используются при выводе, в рабочей памяти только указатели.
quadLookup:
    ld a,(quadUseFull)
    or a
    jp nz,quadFullLookup
    ld a,(cacheIndexPage+1)
    add a,b
    ld h,a
    ld l,c
    ld a,(hl)
    or a
    jp z,quadMiss
    ; Срок жизни и декодирование номера без стека и CALL на hit.
    ld b,a
    ld a,(cacheLifeRecent)
    ld (cacheLifeOlder),a
    ld a,b
    ld (cacheLifeRecent),a
    rrca
    rrca
    rrca
    rrca
    ld l,a
    and 00Fh
    ld h,a
    ld a,(cachePoolPage+1)
    add a,h
    ld h,a
    ld a,l
    and 0F0h
    ld l,a
    ret
quadMiss:
    call cacheMiss
    ld a,(cacheLastToken)
quadProtect:
    ld b,a
    ld a,(cacheLifeRecent)
    ld (cacheLifeOlder),a
    ld a,b
    ld (cacheLifeRecent),a
    ret
quadFullLookup:
    ld h,0
    ld l,c
    add hl,hl
    add hl,hl
    add hl,hl
    ld a,b
    or a
    jp z,quadFull0
    add hl,hl
    dec a
    ld a,040h
    jp z,quadFullBase
    ld a,050h
    jp quadFullBase
quadFull0:
    ld a,030h
quadFullBase:
    add a,h
    ld h,a
    ret
quadP0: DW 0
quadP6: DW 0
quadP4: DW 0
quadRaw: DW 0
quadText: DW 0
quadRemaining: DB 0
quadUseFull: DB 0
    INCLUDE "quad-direct.inc"
