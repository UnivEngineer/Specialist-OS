; Orion-128 Monitor M1, m1rk.bin FCFAh-FD25h.
; Keep the paired DAD H shifts, masked merge and eight-row loop.
; FontBen supplies its common KOI8 font, text frontends and screen geometry.
    INCLUDE "../../sdk/asm/mxos.inc"
IMAGE_FLAGS = IMAGE_FLAG_MAIN
    ORG 02000h
    jp initialize
descriptor:
    DB 6,1
    DW scalar,batch,empty

initialize:
    ld hl,descriptor
    xor a
empty:
    ret

; C is the glyph code. The screen address uses column in H and row in L.
scalar:
    ld l,c
    ld h,0
    add hl,hl
    add hl,hl
    add hl,hl
    ld de,font
    add hl,de
    push hl

    ld hl,(bios_vars.cursorY)
    ld c,h
    ld a,h
    add a,3
    ld (bios_vars.cursorX),a
    ld a,h
    rrca
    rrca
    and 3Fh
    add a,90h
    ld h,a
    ld a,l
    sub 7
    ld l,a
    ex de,hl

    ld a,c
    and 3
    ld c,a
    sub 4                      ; 1 - (5 - phase), as in the M1 counter.
    ld (shiftPairs),a
    ld l,c
    ld h,0
    add hl,hl
    ld bc,masks
    add hl,bc
    ld b,(hl)
    inc hl
    ld c,(hl)
    pop hl
    ld a,d
    cp 0BFh
    ld a,22
    jp z,lastColumn

    MACRO M1_ROWS entry,rightByte
entry:
    push af
    push hl
    ld a,(bios_vars.inverse)
    xor (hl)
    and 3Fh
    ld l,a
    ld a,(shiftPairs)
    dec a
    ld h,0
.shift:
    add hl,hl
    add hl,hl
    inc a
    jp nz,.shift
    ex de,hl
    ld a,b
    xor (hl)
    and (hl)
    or d
    ld (hl),a
    IF rightByte
    inc h
    ld a,c
    xor (hl)
    and (hl)
    or e
    ld (hl),a
    dec h
    ENDIF
    inc l
    ex de,hl
    pop hl
    inc hl
    pop af
    sub 3
    jp p,entry
    ret
    ENDM

    M1_ROWS rows,1
    ; BFxx is the last screen column. Any spill lies outside the display.
    M1_ROWS lastColumn,0

; The original monitor draws individual glyphs. The shared string frontends
; pass a bounded run here. Save the text iterator across the scalar call.
batch:
    push af
    push bc
    push de
    push hl
    ld b,c
.next:
    push bc
    push hl
    ld c,(hl)
    call scalar
    pop hl
    pop bc
    inc hl
    dec b
    jp nz,.next
    pop hl
    pop de
    pop bc
    pop af
    ret
shiftPairs: DB 0
masks: DB 0FCh,0,03Fh,0,00Fh,0C0h,003h,0F0h
font: INCBIN "../../assets/system-rom/fonts/6x8/KOI8.pf"

