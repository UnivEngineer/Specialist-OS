; Построчный шрифт: восемь страниц по 256 кодов. Регистры сохранены.
; HL=текст, C=длина, DE=экран; длина 6x8 кратна четырём. IRQ выключены.
    ASSERT (PAGED_FONT_BASE & 0FFh) = 0
    ASSERT (PAGED_PRE_BASE & 0FFh) = 0
    IFNDEF PAGED_CUSTOM_PHASES
PAGED_PHASE0 = PAGED_PRE_BASE
PAGED_PHASE1 = PAGED_PRE_BASE+00800h
PAGED_PHASE2 = PAGED_PRE_BASE+01000h
PAGED_PHASE3 = PAGED_PRE_BASE+01800h
PAGED_PHASE4 = PAGED_PRE_BASE+02000h
    ENDIF
    MODULE paged
    MACRO PAGE_ROW kind, advance
    ld a,(bc)
    IF kind = 0
    DUP 4
        rrca
    EDUP
    and 3
    ENDIF
    IF kind = 1
    rrca
    rrca
    and 0Fh
    ENDIF
    IF kind < 3
    ld (.merge+1),a
    ld a,(hl)
    IF kind = 0
    add a,a
    add a,a
    ENDIF
    IF kind = 1
    DUP 4
        rrca
    EDUP
    and 0F0h
    ENDIF
    IF kind = 2
    rrca
    rrca
    and 0C0h
    ENDIF
.merge: or 0
    ELSE
    or (hl)
    ENDIF
    ld (de),a
    IF advance
    inc b
    inc h
    inc e
    ENDIF
    ENDM
    MACRO PAGE_COLUMN offset, kind
    ld hl,(codes+offset)
    ld c,h
    ; Compile only the two required page loads. Pre-rendered columns must
    ; not first load font pages that are immediately overwritten below.
    IF kind < 3
    ld h,PAGED_FONT_BASE/256
    ld b,h
    ELSE
    IF kind = 3
    ld h,PAGED_PHASE0/256
    ld b,PAGED_PHASE3/256
    ENDIF
    IF kind = 4
    ld h,PAGED_PHASE4/256
    ld b,PAGED_PHASE1/256
    ENDIF
    IF kind = 5
    ld h,PAGED_PHASE2/256
    ld b,PAGED_FONT_BASE/256
    ENDIF
    ENDIF
    ; The last row needs no source/screen pointer increments.
    DUP 7
        PAGE_ROW kind,1
    EDUP
    PAGE_ROW kind,0
    ld a,e
    sub 7
    ld e,a
    inc d
    ENDM
; Independent kernels: only initialization selects the entry point.
; No mode byte, conditional branch or patched dispatch inside either loop.
raw6:
    push af
    push bc
    push de
    push hl
    call raw6Helper
    jp done
pre6:
    push af
    push bc
    push de
    push hl
    call pre6Helper
    jp done
    IFDEF FONT_REMAP32
    MACRO REMAP_CODE offset
    ld a,(hl)
    rlca
    rlca
    rlca
    and 7
    or blockMap & 0FFh
    ld c,a
    ld a,(bc)
    ld c,a
    ld a,(hl)
    and 01Fh
    or c
    ld (codes+offset),a
    inc hl
    ENDM
    ENDIF
raw6Helper:
    ld a,c
    or a
    ret z
    ld (remaining),a
    ld (text),hl
raw6Next:
    ld hl,(text)
    IFDEF FONT_REMAP32
    ld b,blockMap >> 8
    REMAP_CODE 0
    REMAP_CODE 1
    REMAP_CODE 2
    REMAP_CODE 3
    ELSE
    ld a,(hl)
    ld (codes+0),a
    inc hl
    ld a,(hl)
    ld (codes+1),a
    inc hl
    ld a,(hl)
    ld (codes+2),a
    inc hl
    ld a,(hl)
    ld (codes+3),a
    inc hl
    ENDIF
    ld (text),hl
    PAGE_COLUMN 0,0
    PAGE_COLUMN 1,1
    PAGE_COLUMN 2,2
    ld a,(remaining)
    sub 4
    ld (remaining),a
    jp nz,raw6Next
    ret
pre6Helper:
    ld a,c
    or a
    ret z
    ld (remaining),a
    ld (text),hl
pre6Next:
    ld hl,(text)
    IFDEF FONT_REMAP32
    ld b,blockMap >> 8
    REMAP_CODE 0
    REMAP_CODE 1
    REMAP_CODE 2
    REMAP_CODE 3
    ELSE
    ld a,(hl)
    ld (codes+0),a
    inc hl
    ld a,(hl)
    ld (codes+1),a
    inc hl
    ld a,(hl)
    ld (codes+2),a
    inc hl
    ld a,(hl)
    ld (codes+3),a
    inc hl
    ENDIF
    ld (text),hl
    PAGE_COLUMN 0,3
    PAGE_COLUMN 1,4
    PAGE_COLUMN 2,5
    ld a,(remaining)
    sub 4
    ld (remaining),a
    jp nz,pre6Next
    ret
raw8:
    push af
    push bc
    push de
    push hl
    call raw8Helper
    jp done
raw8Helper:
    ld a,c
    or a
    ret z
    ld (remaining),a
    ; BC stays the text cursor while HL walks the eight font row pages.
    ; No LHLD/SHLD of a text cursor per glyph (32 T saved each time).
    ld b,h
    ld c,l
next8:
    ld a,(bc)
    inc bc
    ld l,a
    ld h,PAGED_FONT_BASE/256
    DUP 7
        ld a,(hl)
        ld (de),a
        inc h
        inc e
    EDUP
    ; The final row is terminal: neither pointer needs an eighth increment.
    ld a,(hl)
    ld (de),a
    ld a,e
    sub 7
    ld e,a
    inc d
    ld hl,remaining
    dec (hl)
    jp nz,next8
    ret

done:
    pop hl
    pop de
    pop bc
    pop af
    ret
text: dw 0
codes: ds 4
remaining: db 0
    IFDEF FONT_REMAP32
    ALIGN 8,0
blockMap: DB 00h,20h,40h,60h,80h,0A0h,0C0h,0E0h
    ENDIF
    ENDMODULE
