; HL=IO_PROG_A, DE=буфер ровно 256 байт; регистры сохраняет внешний Driver.
; При E=0 младший адрес совпадает с E: STAX B вместо STA, INC E вместо INX D.
; Быстрый цикл x8: 31 + 10/8 = 32,25 такта/байт; общий путь — 52.
    IFNDEF PIO_UNALIGNED_MODE
    DEFINE PIO_UNALIGNED_MODE 0
    ENDIF
    IFNDEF PIO_UNALIGNED_UNROLL
    DEFINE PIO_UNALIGNED_UNROLL 8
    ENDIF
    IF PIO_UNALIGNED_MODE != 3
    ld a,e
    or a
    jp nz,ReadUnaligned
    ld bc,IO_PROG_B
ReadAlignedLoop:
    DUP 8
    ld a,e
    ld (bc),a
    ld a,(hl)
    ld (de),a
    inc e
    EDUP
    jp nz,ReadAlignedLoop
    jp ReadDone
    ENDIF
ReadUnaligned:
    IF PIO_UNALIGNED_MODE == 2
    ; E — младший адрес RAM, E-start — адрес байта flash. Исправляем восемь
    ; ADI только при смене смещения; затем STAX B доступен без выравнивания.
    ld a,e
ReadCachedLow:
    cp 0
    jp z,ReadOffsetReady
    ld (ReadCachedLow+1),a
    ld (ReadOffsetEnd+1),a
    cpl
    inc a
    ld (ReadOffsetLoop+2),a
    ld (ReadOffsetLoop+9),a
    ld (ReadOffsetLoop+16),a
    ld (ReadOffsetLoop+23),a
    ld (ReadOffsetLoop+30),a
    ld (ReadOffsetLoop+37),a
    ld (ReadOffsetLoop+44),a
    ld (ReadOffsetLoop+51),a
ReadOffsetReady:
    ld bc,IO_PROG_B
ReadOffsetLoop:
    DUP 8
    ld a,e
    add a,0
    ld (bc),a
    ld a,(hl)
    ld (de),a
    inc de
    EDUP
    ld a,e
ReadOffsetEnd:
    cp 0
    jp nz,ReadOffsetLoop
ReadLoop = ReadOffsetLoop
    ELSE
    ld c,0
ReadLoop:
ReadRepeat = 1
    IF PIO_UNALIGNED_MODE == 1 || PIO_UNALIGNED_MODE == 3
ReadRepeat = PIO_UNALIGNED_UNROLL
    ENDIF
    DUP ReadRepeat
    ld a,c
    ld (IO_PROG_B),a
    ld a,(hl)
    ld (de),a
    inc de
    inc c
    EDUP
    jp nz,ReadLoop
    ENDIF
ReadDone:
    IF PIO_UNALIGNED_MODE == 3
ReadAlignedLoop = ReadLoop
    ENDIF
