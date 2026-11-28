ramfosSetCursor:
    push af
    push bc
    push hl
    ld a,h
    add a,a
    ld h,a
    add a,a
    add a,a
    add a,h
    add a,8
    ld h,a
    ld a,(FONT_ENGINE_HEADER+5)
    rrca
    ld b,a
    xor a
.column:
    add a,l
    dec b
    jp nz,.column
    ld l,h
    ld h,a
    ld (ramfos_cursorPos),hl
    ld (mon2_vars.cursorY),hl
    pop hl
    pop bc
    pop af
    ret

    INCLUDE "ramfos-file-stubs.inc"
