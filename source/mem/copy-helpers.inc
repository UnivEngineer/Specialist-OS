    IFDEF COPY_HELPER_BYTES
memCopyBytesBody:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,memCopyBytesBody
    ret

    ELSE
; Известные пакеты 8/32 байта: B=число четвёрок, HL -> DE.
; Произвольный размер по-прежнему обслуживает memCopyBytesBody.
memCopyQuadsBody:
    DUP 4
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    EDUP
    dec b
    jp nz,memCopyQuadsBody
    ret

    ENDIF
