; Attach only consumes already prepared resources. No DOS calls or allocation.
; DE=lower packed planes, or zero for a rotating module.
attach:
    ; Reattach is idempotent even after a physical KOI7 switch.
    xor a
    call setEncoding
    ex de,hl
    ld (fontPreBase),hl
    ex de,hl
    IFDEF FONT4_FAST
    ld a,d
    ld (fast6.normalZRightPage+1),a
    ld (fast6.normalRawZRightPage+1),a
    ld (fast6.normalLimRightPage+1),a
    ld (fast6.inverseZRightPage+1),a
    ld (fast6.inverseRawZRightPage+1),a
    ld (fast6.inverseLimRightPage+1),a
    ENDIF
    ld hl,FONT_ADDR
    ld (bios_upper.charGen),hl
    xor a
    ld (bios_upper.koi7),a
    ld (encodingOld+1),a
    IFDEF FONT4_COMPACT
    call refreshCompactReverse
    ENDIF
    IF FONT_MODE
    MACRO COLUMN_PAGES normal, inverse, upNormal, upInverse
    ld (normal+1),a
    ld (inverse+1),a
    add a,7
    ld (upNormal+1),a
    ld (upInverse+1),a
    sub 7
    ENDM
    ld a,d
    COLUMN_PAGES fast6.pre0Normal,fast6.pre0,fast6.pre0NormalUp,fast6.pre0Up
    IF FONT_MODE == 1
    COLUMN_PAGES fast6.pre1Normal,fast6.pre1,fast6.pre1NormalUp,fast6.pre1Up
    ld (packedNormal.page0_0+1),a
    ld (packedNormal.page0_1+1),a
    ld (packedInverse.page0_0+1),a
    ld (packedInverse.page0_1+1),a
    add a,8
    COLUMN_PAGES fast6.pre2Normal,fast6.pre2,fast6.pre2NormalUp,fast6.pre2Up
    COLUMN_PAGES fast6.pre3Normal,fast6.pre3,fast6.pre3NormalUp,fast6.pre3Up
    ld (packedNormal.page1_0+1),a
    ld (packedNormal.page1_1+1),a
    ld (packedInverse.page1_0+1),a
    ld (packedInverse.page1_1+1),a
    ld a,FONT_ADDR >> 8
    COLUMN_PAGES fast6.pre4Normal,fast6.pre4,fast6.pre4NormalUp,fast6.pre4Up
    ld (packedNormal.page2_0+1),a
    ld (packedNormal.page2_1+1),a
    ld (packedInverse.page2_0+1),a
    ld (packedInverse.page2_1+1),a
    ELSE
    add a,24
    COLUMN_PAGES fast6.pre1Normal,fast6.pre1,fast6.pre1NormalUp,fast6.pre1Up
    add a,8
    COLUMN_PAGES fast6.pre2Normal,fast6.pre2,fast6.pre2NormalUp,fast6.pre2Up
    sub 24
    COLUMN_PAGES fast6.pre3Normal,fast6.pre3,fast6.pre3NormalUp,fast6.pre3Up
    add a,8
    COLUMN_PAGES fast6.pre4Normal,fast6.pre4,fast6.pre4NormalUp,fast6.pre4Up
    ld a,FONT_ADDR >> 8
    ENDIF
    COLUMN_PAGES fast6.pre5Normal,fast6.pre5,fast6.pre5NormalUp,fast6.pre5Up
    ENDIF
    ; Publish only historical/new public BIOS JMP operands. Internal BIOS and
    ; MXCALL targets always enter the fixed D800h table and need no rewriting.
    INCLUDE "bind.inc"
    xor a
    ret
; Переставляем 31 букву. DEL стирает знакоместо в KOI-7R, FFh хранит Ъ.
setEncoding:
    push af
    push bc
    push de
    push hl
    or a
    jp z,encodingNormalized
    ld a,0FFh
encodingNormalized:
encodingOld: cp 0
    jp z,encodingDone
    ld (encodingOld+1),a
    ld (bios_upper.koi7),a
    IFDEF FONT4_COMPACT
    ld hl,FONT_ADDR+0180h
    ld de,FONT_ADDR+0380h
    ld c,120
    call swapByte
    ; Последняя запись хранит 7Eh/7Fh и FEh/FFh. Меняем только старшие части.
    ld b,0F0h
    call swapCompactNibbles
    ld b,0Fh
    ld l,0F8h
    ld de,encodingDel
    call swapCompactNibbles
    call refreshCompactReverse
    ELSE
    ld de,encodingDel
    ld h,FONT_ADDR >> 8
    ld b,8
    call swapPages
    IF FONT_LOWER_BYTES
    ld hl,(fontPreBase)
    ld b,FONT_LOWER_BYTES/256
    call swapPages
    ENDIF
    ENDIF
encodingDone:
    pop hl
    pop de
    pop bc
    pop af
    ret
    IFNDEF FONT4_COMPACT
; H — первая страница, B — число страниц, DE — сохранённые байты DEL.
swapPages:
    push de
    ld d,h
    ld l,60h
    ld e,0E0h
    ld c,31
    call swapByte
    pop de
    ; HL уже указывает на 7Fh. Обмен сохраняет и восстанавливает DEL.
    ld c,(hl)
    ld a,(de)
    ld (hl),a
    ld a,c
    ld (de),a
    inc de
    inc h
    dec b
    jp nz,swapPages
    ret
    ENDIF
swapByte:
    ld a,(de)
    ld (swapSaved+1),a
    ld a,(hl)
    ld (de),a
swapSaved: ld (hl),0
    inc l
    inc e
    dec c
    jp nz,swapByte
    ret

    IFDEF FONT4_COMPACT
; HL/DE указывают на восьмибайтные записи, B — маска обмена.
swapCompactNibbles:
    ld a,(de)
    xor (hl)
    and b
    ld c,a
    xor (hl)
    ld (hl),a
    ld a,(de)
    xor c
    ld (de),a
    inc de
    inc l
    jp nz,swapCompactNibbles
    ret

; Дубликат занимает 1 КиБ в выравнивании кода перед основным шрифтом.
; В каждой записи пары глифов строки идут от нижней к верхней.
refreshCompactReverse:
    IFDEF FONT_BENCH_STREAM
    ld a,(benchStream)
    or a
    ret nz
    ENDIF
    ld hl,FONT_ADDR
    ld de,FONT4_REVERSE_ADDR+7
    ld b,128
compactCopyPair:
    DUP 7
    ld a,(hl)
    ld (de),a
    inc hl
    dec e
    EDUP
    ld a,(hl)
    ld (de),a
    inc hl
    ld a,e
    add a,15
    ld e,a
    jp nc,compactCopyNext
    inc d
compactCopyNext:
    dec b
    jp nz,compactCopyPair
    ret
    ENDIF
encodingDel: DS 8+FONT_LOWER_BYTES/256,0

; The module owns its mode. Switching is an installation, not a hot flag.
service:
    or a
    jp z,query
    cp FONT_RENDER_DISABLE
    jp nz,unsupported
    IF FONT_LOWER_BYTES
    jp unsupported
    ENDIF
query:
    ld hl,(fontPreBase)
    ld de,FONT_ADDR
    ld bc,FONT_LOWER_BYTES
    IF FONT_LOWER_BYTES
    ld a,1
    ELSE
    xor a
    ENDIF
    or a
    ret
unsupported:
    ld a,DOS_ERR_UNSUPPORTED
    scf
    ret
fontPreBase: DW 0
