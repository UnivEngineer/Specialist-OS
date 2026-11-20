; Rebuild text geometry when a byte edit changed a line separator.
hexRestoreTextGeometry:
    ld a,(hexChangedBreak)
    or a
    ret z
    ld hl,(v_gapLo)
    ex de,hl
    ld hl,(v_textBeginLo)
    call editorCountSeparators
    push bc
    ld hl,(v_textLimitLo)
    dec hl
    ex de,hl
    ld hl,(v_gapHi)
    call editorCountSeparators
    ld h,b
    ld l,c
    pop de
    add hl,de
    push hl
    ld hl,(v_gapLo)
    call editorInsideCRLF
    pop hl
    jp z,hexStoreTextLines      ; one of the two counts belongs to CRLF
    inc hl
hexStoreTextLines:
    call editorStoreLineCount
    ld hl,(v_currentLine)
    ld (v_topLine),hl
    ret
