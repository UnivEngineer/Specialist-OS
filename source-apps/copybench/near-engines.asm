; Автономные near-кандидаты. Не включаются в BIOS.
; HL=источник, DE=назначение, BC=размер у byteCopy; диапазоны не пересекаются.
; Остальные входы: ровно 256 байт. AF/BC/DE/HL разрушены, SP сохранён.
    ORG 08000h
byteCopy:
    ld a,b
    or c
    ret z
byteLoop:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,byteLoop
    ret

; Только назначение выровнено; источник произвольный, с переносом в H.
alignedByte256:
alignedByteLoop:
    DUP 8
    ld a,(hl)
    ld (de),a
    inc hl
    inc e
    EDUP
    jp nz,alignedByteLoop
    ret

stack256:
    ld (stackReadSP+1),hl
    ld hl,0
    add hl,sp
    ld (stackRestoreSP+1),hl
    ex de,hl
    ld b,16
stackReadSP:
    ld sp,0
stackLoop:
    DUP 8
    pop de
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    EDUP
    dec b
    jp nz,stackLoop
stackRestoreSP:
    ld sp,0
    ret

; Назначение выровнено. INC L стоит столько же, сколько INX H,
; но даёт Z для окончания сектора и освобождает отдельный счётчик B.
alignedStack256:
    ld (alignedReadSP+1),hl
    ld hl,0
    add hl,sp
    ld (alignedRestoreSP+1),hl
    ex de,hl
alignedReadSP:
    ld sp,0
alignedStackLoop:
    DUP 8
    pop de
    ld (hl),e
    inc l
    ld (hl),d
    inc l
    EDUP
    jp nz,alignedStackLoop
alignedRestoreSP:
    ld sp,0
    ret
    END
