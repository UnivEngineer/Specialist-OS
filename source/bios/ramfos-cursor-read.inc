; Координаты RAMFOS в H/L измеряются в строках и колонках.
ramfosGetCursor:
    push af
    push bc
    ld a,(FONT_ENGINE_HEADER+5)
    rrca
    ld c,a
    ld a,(ramfos_cursorPos+1)
    ld hl,0FFFFh
.column:
    inc l
    sub c
    jp nc,.column
    ld a,(ramfos_cursorPos)
.row:
    inc h
    sub 10
    jp nc,.row
    pop bc
    pop af
    ret
