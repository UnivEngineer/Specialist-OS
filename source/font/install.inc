; Attach only consumes already prepared resources. No DOS calls or allocation.
; DE=lower packed planes, or zero for a rotating module.
attach:
    ; Reattach is idempotent even after a physical KOI7 switch.
    xor a
    call setEncoding
    ex de,hl
    ld (fontPreBase),hl
    ex de,hl
    ld hl,FONT_ADDR
    ld (bios_upper.charGen),hl
    xor a
    ld (bios_upper.koi7),a
    ld (encodingOld+1),a
    IF FONT_MODE == 1
    ld a,d
    ld (fast6.pre0Normal+1),a
    ld (fast6.pre1Normal+1),a
    ld (fast6.pre0+1),a
    ld (fast6.pre1+1),a
    ld (packedNormal.page0_0+1),a
    ld (packedNormal.page0_1+1),a
    ld (packedInverse.page0_0+1),a
    ld (packedInverse.page0_1+1),a
    add a,8
    ld (fast6.pre2Normal+1),a
    ld (fast6.pre3Normal+1),a
    ld (fast6.pre2+1),a
    ld (fast6.pre3+1),a
    ld (packedNormal.page1_0+1),a
    ld (packedNormal.page1_1+1),a
    ld (packedInverse.page1_0+1),a
    ld (packedInverse.page1_1+1),a
    ld a,FONT_ADDR >> 8
    ld (fast6.pre4Normal+1),a
    ld (fast6.pre5Normal+1),a
    ld (fast6.pre4+1),a
    ld (fast6.pre5+1),a
    ld (packedNormal.page2_0+1),a
    ld (packedNormal.page2_1+1),a
    ld (packedInverse.page2_0+1),a
    ld (packedInverse.page2_1+1),a
    ENDIF
    IF FONT_MODE == 2
    ld a,d
    ld (fast6.pre0+1),a
    ld (fast6.pre0Normal+1),a
    add a,24
    ld (fast6.pre1+1),a
    ld (fast6.pre1Normal+1),a
    add a,8
    ld (fast6.pre2+1),a
    ld (fast6.pre2Normal+1),a
    sub 24
    ld (fast6.pre3+1),a
    ld (fast6.pre3Normal+1),a
    add a,8
    ld (fast6.pre4+1),a
    ld (fast6.pre4Normal+1),a
    ld a,FONT_ADDR >> 8
    ld (fast6.pre5+1),a
    ld (fast6.pre5Normal+1),a
    ENDIF
    ; Publish only historical/new public BIOS JMP operands. Internal BIOS and
    ; MXCALL targets always enter the fixed D800h table and need no rewriting.
    INCLUDE "bind.inc"
    xor a
    ret
; Encoding changes physically reorder the selected font and all its planes.
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
    ld h,FONT_ADDR >> 8
    ld b,8
    call swapPages
    IF FONT_MODE
    ld hl,(fontPreBase)
    ld b,FONT_LOWER_BYTES/256
    call swapPages
    ENDIF
encodingDone:
    pop hl
    pop de
    pop bc
    pop af
    ret
swapPages:
    ld d,h
    ld l,60h
    ld e,0E0h
    ld c,32
swapByte:
    ld a,(de)
    ld (swapSaved+1),a
    ld a,(hl)
    ld (de),a
swapSaved: ld a,0
    ld (hl),a
    inc l
    inc e
    dec c
    jp nz,swapByte
    inc h
    dec b
    jp nz,swapPages
    ret
; The module owns its mode. Switching is an installation, not a hot flag.
service:
    or a
    jp z,query
    cp FONT_RENDER_DISABLE
    jp nz,unsupported
    IF FONT_MODE
    jp unsupported
    ENDIF
query:
    ld hl,(fontPreBase)
    ld de,FONT_ADDR
    ld bc,FONT_LOWER_BYTES
    IF FONT_MODE
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
