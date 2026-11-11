; Упакованный FONTKOI: 32 байта заголовка, 512 байт указателей, 147*24.
; Подготовка вне замера; относительные указатели превращаются в абсолютные.
; Исходная фаза восстанавливается двумя RRCA из готовой фазы 0.
quadPreparePacked:
    call cacheDiscard
    ld hl,0
    ld (packedBase),hl
    xor a
    IFDEF FONT_ENGINE_DLL
    ld hl,04000h
    ld bc,01000h
    ELSE
    MXOS_CALL MXOS_BIOS_GLYPH_CACHE
    ret c
    ENDIF
    ld a,b
    cp 00Fh
    jp c,packedUseMain
    jp nz,packedSpaceReady
    ld a,c
    cp 0E8h
    jp nc,packedSpaceReady
; При росте BIOS верхняя арена может стать меньше 4072 байт.
; Временно занимаем фазу 4 полного пререндера; матрица восстановит её.
packedUseMain:
    ld hl,04000h
    ld a,1
    ld (packedOverlay),a
packedSpaceReady:
    ld (packedBase),hl
    push hl
    ld bc,32
    add hl,bc
    ld (packedLookupBase+1),hl
    pop hl
    ex de,hl
    ld hl,packedHeader
    ld bc,32
    call bench_copy
    ex de,hl
    ld (packedWrite),hl
    ld hl,packedIndex
    ld (packedRead+1),hl
    ld a,0
    ld (packedCount),a
packedPointer:
packedRead:
    ld hl,packedIndex
    ld a,(hl)
    inc hl
    ld (packedRead+1),hl
    ld l,a
    ld h,0
    ld b,h
    ld c,l
    add hl,hl
    add hl,bc
    add hl,hl
    add hl,hl
    add hl,hl
    ld bc,544
    add hl,bc
    push hl
    ld hl,(packedBase)
    ld b,h
    ld c,l
    pop hl
    add hl,bc
    ex de,hl
    ld hl,(packedWrite)
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    ld (packedWrite),hl
    ld hl,packedCount
    dec (hl)
    jp nz,packedPointer
    ld hl,(packedWrite)
    ex de,hl
    ld hl,packedCodes
    ld (packedCodeRead+1),hl
    ld a,147
    ld (packedCount),a
packedGlyph:
packedCodeRead:
    ld hl,packedCodes
    ld c,(hl)
    ld b,0
    inc hl
    ld (packedCodeRead+1),hl
    ld hl,02800h/8
    add hl,bc
    add hl,hl
    add hl,hl
    add hl,hl
    ld (packedSource+1),hl
    ld b,0
packedPhase:
packedSource:
    ld hl,0
    ld c,8
packedBuildRow:
    ld a,(hl)
    inc hl
    push af
    ld a,b
    or a
    jp nz,packedBuildRotate
    pop af
    add a,a
    add a,a
    jp packedBuildStore
packedBuildRotate:
    cp 1
    jp z,packedBuildRot2
    pop af
    rrca
    rrca
    rrca
    rrca
    jp packedBuildStore
packedBuildRot2:
    pop af
    rrca
    rrca
packedBuildStore:
    ld (de),a
    inc de
    dec c
    jp nz,packedBuildRow
    inc b
    ld a,b
    cp 3
    jp nz,packedPhase
    ld hl,packedCount
    dec (hl)
    jp nz,packedGlyph
    or a
    ret

packedLookup:
    ld l,a
    ld h,0
    add hl,hl
packedLookupBase:
    ld bc,0
    add hl,bc
    ld c,(hl)
    inc hl
    ld h,(hl)
    ld l,c
    ret
quadPackedGroup:
    ld hl,(packedBase)
    ld a,h
    or l
    jp z,quadError
    ld hl,(quadText)
    ex de,hl
    ld a,(de)
    inc de
    call packedLookup
    ld (quadP0),hl
    ld a,(de)
    inc de
    call packedLookup
    ld bc,16
    add hl,bc
    ld (quadP6),hl
    ld a,(de)
    inc de
    call packedLookup
    ld bc,8
    add hl,bc
    ld (quadP4),hl
    ld a,(de)
    inc de
    call packedLookup
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
    ld (packedRestore+1),hl
    ld a,2
    ld (packedCount),a
packedPixels:
    DUP 4
    ld sp,quadP0
    pop hl
    ld a,(hl)
    inc hl
    ld (quadP0),hl
    ld b,a
    pop hl
    ld a,(hl)
    inc hl
    ld (quadP6),hl
    ld c,a
    and 3
    or b
    ld (de),a
    inc d
    ld a,c
    and 0F0h
    ld b,a
    pop hl
    ld a,(hl)
    inc hl
    ld (quadP4),hl
    ld c,a
    and 00Fh
    or b
    ld (de),a
    inc d
    ld a,c
    and 0C0h
    ld b,a
    pop hl
    ld a,(hl)
    inc hl
    ld (quadRaw),hl
    rrca
    rrca
    or b
    ld (de),a
    dec d
    dec d
    inc e
    EDUP
    ld hl,packedCount
    dec (hl)
    jp nz,packedPixels
packedRestore:
    ld sp,0
    jp quadPixelsEnd
packedBase: DW 0
packedWrite: DW 0
packedCount: DB 0
packedHeader: INCBIN "../../../bin/FONTKOI.packed.bin",0,32
packedIndex: INCBIN "../../../bin/FONTKOI.packed.index.bin"
packedCodes: INCBIN "../../../bin/FONTKOI.packed.codes.bin"
