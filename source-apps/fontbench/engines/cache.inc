; Кеш по паре (код, фаза). Строится только запрошенный сдвиг.
; FONT.FNT: линейные глифы 256x8, шесть бит выровнены вправо.
; Фаза X mod 8 = 2 читает шрифт напрямую, без кеша и генерации.
    IFNDEF FONT_ENGINE_DLL
    INCLUDE "../../../bin/FONTBEN.renderer.entries.inc"
    ENDIF

; C=код; все регистры/флаги сохранены, координаты как у SHIFT MODE=1.
cacheRender:
    push af
    push bc
    push de
    push hl
cacheBody:
    ; Исходной фазе не нужны ни Query, ни проверка/инициализация кеша.
    ld hl,(bios_vars.cursorY)
    ld a,h
    and 3
    cp 1
    jp z,cacheReady
    ld a,(GLYPH_CACHE_INVALID)
    or a
    call nz,cachePrepare
cacheMode:
    jp cacheFallback
cacheReady:
    ld hl,(bios_vars.cursorY)
    ld b,h
    inc h
    inc h
    inc h
    ld (bios_vars.cursorY),hl
    ld h,b
    ld a,h
    and 3
    ld b,a
    ld a,h
    rrca
    rrca
    and 3Fh
    add a,090h
    ld h,a
    ld a,l
    sub 7
    ld l,a
    ex de,hl
    ld a,b
    cp 1
    jp z,cacheRaw
    ; Физические фазы 0/4/6 -> страницы индекса 0/1/2.
    or a
    jp z,cacheLookup
    dec a
cacheLookup:
cacheIndexPage:
    add a,0
    ld h,a
    ld l,c
    ld a,(hl)
    or a
    jp z,cacheAbsent
    ; Номер 0 — промах; token*16+pool, без полного указателя.
    rrca
    rrca
    rrca
    rrca
    ld l,a
    and 00Fh
cachePoolPage:
    add a,0
    ld h,a
    ld a,l
    and 0F0h
    ld l,a
cachePhase:
    ld a,b
    or a
    jp z,cachePhase0
    cp 2
    jp z,cachePhase4
    ld bc,0FC0Fh
    jp CACHE_TWOBYTES
cachePhase4:
    ld bc,0F03Fh
    jp CACHE_TWOBYTES
cachePhase0:
    ld c,3
    jp CACHE_ONEBYTE
cacheRaw:
    ld hl,(renderFont8)
    ld b,0
    add hl,bc
    add hl,hl
    add hl,hl
    add hl,hl
    ld c,0C0h
    jp CACHE_ONEBYTE
cacheFallback:
cacheFallbackExit:
    jp cacheFallbackScalar
cacheFallbackScalar:
    pop hl
    pop de
    pop bc
    pop af
    jp monitor.scalar
cacheAbsent:
    call cacheMiss
    jp cachePhase

; После инвалидирования: 3x256 lookup, 2x256 обратные теги, затем пул.
; Нулевой 16-байтный блок пула запрещён: token=0 обозначает промах.
; AF/BC/DE/HL сохранены; неполные блоки за limit не используются.
cachePrepare:
    push af
    push bc
    push de
    push hl
    ld hl,cacheFallback
    ld (cacheMode+1),hl
    xor a
    ld (cacheSlots),a
    ld (cacheFilled),a
    inc a
    ld (cacheVictim),a
    ld hl,0
    ld (GLYPH_CACHE_ENTRIES),hl
    xor a
    IFDEF FONT_ENGINE_DLL
    ld hl,03000h
    ld de,03800h
    ld bc,0800h
    ELSE
    MXOS_CALL MXOS_BIOS_GLYPH_CACHE
    jp c,cachePrepared
    ENDIF
    ld (cacheEpoch),a
    push hl
    ld hl,(cachePrivateStart)
    ld a,h
    or l
    pop hl
    jp z,cacheArenaReady
    ld hl,(cachePrivateEnd)
    ex de,hl
    ld hl,(cachePrivateStart)
cacheArenaReady:
    ld (cacheArenaStart),hl
    push hl
    ex de,hl
    ld (cacheArenaEnd),hl
    ex de,hl
    pop hl
    ld a,l
    or a
    jp z,cacheAligned
    inc h
cacheAligned:
    ld l,0
    ld (cacheIndex),hl
    ld a,h
    ld (cacheIndexPage+1),a
    add a,3
    ld (cacheOwnersPage),a
    add a,2
    ld h,a
    ld (cachePoolPage+1),a
    ld (cachePool),hl
    ex de,hl
    ld a,l
    sub e
    ld l,a
    ld a,h
    sbc a,d
    ld h,a
    jp c,cachePrepared
    ld b,4
cacheDivide16:
    or a
    ld a,h
    rra
    ld h,a
    ld a,l
    rra
    ld l,a
    dec b
    jp nz,cacheDivide16
    ld a,h
    or a
    jp nz,cacheCap255
    ld a,l
    or a
    jp z,cachePrepared
    dec a
    jp cacheCountReady
cacheCap255:
    ld a,255
cacheCountReady:
    or a
    jp z,cachePrepared
    ld (cacheSlots),a
    ld hl,(cacheIndex)
    ld b,3
    xor a
cacheClearPage:
    ld c,0
cacheClearIndex:
    ld (hl),a
    inc l
    dec c
    jp nz,cacheClearIndex
    inc h
    dec b
    jp nz,cacheClearPage
    ld hl,cacheReady
    ld (cacheMode+1),hl
cachePrepared:
    xor a
    ld (GLYPH_CACHE_INVALID),a
    pop hl
    pop de
    pop bc
    pop af
    ret

; C=код, B=фаза/2, H=страница lookup. Выход HL=подготовленный слот.
; BC/DE сохранены, AF разрушены. FIFO всех сдвинутых фаз общий.
cacheMiss:
    push bc
    push de
    ld a,c
    ld (cacheCode),a
    ld a,h
    ld (cacheBuildPage),a
    ld hl,cacheIndex+1
    sub (hl)
    ld (cacheKeyPhase),a
    ld hl,(cacheMisses)
    inc hl
    ld (cacheMisses),hl
    ld a,(cacheFilled)
    ld hl,cacheSlots
    cp (hl)
    jp c,cacheNewSlot
cacheVictimGuard:
    jp cacheVictimReady
cacheVictimReady:
    ld hl,(cacheEvictions)
    inc hl
    ld (cacheEvictions),hl
    ld a,(cacheVictim)
    ld l,a
    ld a,(cacheOwnersPage)
    ld h,a
    ld c,(hl)
    inc h
    ld a,(hl)
    ld hl,cacheIndex+1
    add a,(hl)
    ld h,a
    ld l,c
    ld (hl),0
    jp cacheOwnerReady
cacheNewSlot:
    ld hl,cacheFilled
    inc (hl)
    ld a,(hl)
    ld l,a
    ld h,0
    ld (GLYPH_CACHE_ENTRIES),hl
cacheOwnerReady:
    ld a,(cacheVictim)
    ld l,a
    ld a,(cacheOwnersPage)
    ld h,a
    ld a,(cacheCode)
    ld (hl),a
    inc h
    ld a,(cacheKeyPhase)
    ld (hl),a
    ld a,(cacheVictim)
    call cacheTokenAddress
    ld (cacheBuildBase),hl
    ex de,hl
    ld hl,(renderFont8)
    ld a,(cacheCode)
    ld c,a
    ld b,0
    add hl,bc
    add hl,hl
    add hl,hl
    add hl,hl
    ld a,(cacheKeyPhase)
    or a
    jp z,cacheBuild0
    dec a
    jp z,cacheBuild4
cacheBuild6:
    ; Глиф выровнен по 8, слот по 16: младшие байты не переполняются.
    DUP 8
    ld a,(hl)
    rrca
    rrca
    rrca
    rrca
    ld c,a
    and 3
    ld (de),a
    inc e
    ld a,c
    and 0F0h
    ld (de),a
    inc e
    inc l
    EDUP
    jp cachePublish
cacheBuild4:
    DUP 8
    ld a,(hl)
    rrca
    rrca
    ld c,a
    and 00Fh
    ld (de),a
    inc e
    ld a,c
    and 0C0h
    ld (de),a
    inc e
    inc l
    EDUP
    jp cachePublish
cacheBuild0:
    DUP 8
    ld a,(hl)
    add a,a
    add a,a
    ld (de),a
    inc e
    inc l
    EDUP
cachePublish:
    ld a,(cacheBuildPage)
    ld h,a
    ld a,(cacheCode)
    ld l,a
    ld a,(cacheVictim)
    ld (hl),a
    ld (cacheLastToken),a
    ld hl,cacheVictim
    ld a,(cacheSlots)
    cp (hl)
    jp z,cacheWrap
    inc (hl)
    jp cacheMissDone
cacheWrap:
    ld (hl),1
cacheMissDone:
    ld hl,(cacheBuildBase)
    pop de
    pop bc
    ret
; A=token, HL=pool+token*16. BC/DE сохранены, AF разрушены.
cacheTokenAddress:
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

; Пачка выдаёт записи срок жизни +2 получения. Два последних номера
; сжато представляют этот срок: они не могут стать жертвами следующих miss.
; При трёх и более слотах незакреплённая жертва всегда существует.
cacheChooseVictim:
    ld a,(cacheVictim)
    ld hl,cacheLifeRecent
    cp (hl)
    jp z,cacheSkipVictim
    inc hl
    cp (hl)
    jp nz,cacheVictimReady
cacheSkipVictim:
    ld hl,cacheVictim
    ld a,(cacheSlots)
    cp (hl)
    jp z,cacheSkipWrap
    inc (hl)
    jp cacheChooseVictim
cacheSkipWrap:
    ld (hl),1
    jp cacheChooseVictim

cacheResetStats:
    ld hl,0
    ld (cacheMisses),hl
    ld (cacheEvictions),hl
    ret
cacheDiscard:
    ld a,1
    ld (GLYPH_CACHE_INVALID),a
    ld hl,0
    ld (GLYPH_CACHE_ENTRIES),hl
    ret
cacheIndex: DW 0
cachePool: DW 0
cacheBuildBase: DW 0
cacheArenaStart: DW 0
cacheArenaEnd: DW 0
cacheMisses: DW 0
cacheEvictions: DW 0
cacheSlots: DB 0
cacheFilled: DB 0
cacheVictim: DB 1
cacheOwnersPage: DB 0
cacheCode: DB 0
cacheKeyPhase: DB 0
cacheBuildPage: DB 0
cacheEpoch: DB 0
cacheLastToken: DB 0
cacheLifeRecent: DB 0
cacheLifeOlder: DB 0

cachePrivateStart: DW 0
cachePrivateEnd: DW 0
