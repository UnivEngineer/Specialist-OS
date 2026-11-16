; HL=IO_PROG_MODE, DE=источник 256 байт, C=старший адрес для DATA polling.
; При E=0 используем BC=порт адреса, E=младший адрес; сохраняем исходный C.
; Выровненный цикл: 57 + 10/PIO_WRITE_UNROLL тактов/байт.
; Произвольный буфер: 68 + 10/PIO_WRITE_UNALIGNED_UNROLL тактов/байт.
; Оба пути укладываются в tBLC=150 мкс при 2 МГц, включая границы итераций.
    IFNDEF PIO_UNALIGNED_MODE
    DEFINE PIO_UNALIGNED_MODE 0
    ENDIF
    IFNDEF PIO_UNALIGNED_UNROLL
    DEFINE PIO_UNALIGNED_UNROLL 8
    ENDIF
    IFNDEF PIO_WRITE_UNROLL
PIO_WRITE_UNROLL = 2
    ENDIF
    IFNDEF PIO_WRITE_UNALIGNED_UNROLL
PIO_WRITE_UNALIGNED_UNROLL = 1
    ENDIF
    ASSERT PIO_WRITE_UNROLL == 1 || PIO_WRITE_UNROLL == 2 || PIO_WRITE_UNROLL == 4 || PIO_WRITE_UNROLL == 8 || PIO_WRITE_UNROLL == 16 || PIO_WRITE_UNROLL == 32 || PIO_WRITE_UNROLL == 64 || PIO_WRITE_UNROLL == 128 || PIO_WRITE_UNROLL == 256
    ASSERT PIO_WRITE_UNALIGNED_UNROLL == 1 || PIO_WRITE_UNALIGNED_UNROLL == 2 || PIO_WRITE_UNALIGNED_UNROLL == 4 || PIO_WRITE_UNALIGNED_UNROLL == 8 || PIO_WRITE_UNALIGNED_UNROLL == 16 || PIO_WRITE_UNALIGNED_UNROLL == 32 || PIO_WRITE_UNALIGNED_UNROLL == 64 || PIO_WRITE_UNALIGNED_UNROLL == 128 || PIO_WRITE_UNALIGNED_UNROLL == 256
WriteStart:
    IF PIO_UNALIGNED_MODE != 3
    ld a,e
    or a
    jp nz,WriteUnaligned
    push bc
    ld bc,IO_PROG_B
WriteAlignedLoop:
    DUP PIO_WRITE_UNROLL
    ld a,e
    ld (bc),a
    ld a,(de)
    ld (IO_PROG_A),a
    ld (hl),WRITE_0
    ld (hl),WRITE_1
    inc e
    EDUP
    jp nz,WriteAlignedLoop
    inc d                       ; DE=исходный буфер+256, как в общем пути.
    pop bc
    jp WriteDone
    ENDIF
WriteUnaligned:
    IF PIO_UNALIGNED_MODE == 2
    push bc
    ld bc,IO_PROG_B
WriteOffsetLoop:
    DUP 8
    ld a,e
    add a,0
    ld (bc),a
    ld a,(de)
    ld (IO_PROG_A),a
    ld (hl),WRITE_0
    ld (hl),WRITE_1
    inc de
    EDUP
    ld a,e
WriteOffsetEnd:
    cp 0
    jp nz,WriteOffsetLoop
    pop bc
    jp WriteDone
WriteLoop = WriteOffsetLoop
; Обязательно до SDP: патч кода между последней командой SDP и первым
; стробом данных мог бы превысить tBLC. BC/DE/HL сохранены, AF разрушены.
Prepare:
    ld a,l
WriteCachedLow:
    cp 0
    ret z
    ld (WriteCachedLow+1),a
    ld (WriteOffsetEnd+1),a
    cpl
    inc a
    ld (WriteOffsetLoop+2),a
    ld (WriteOffsetLoop+15),a
    ld (WriteOffsetLoop+28),a
    ld (WriteOffsetLoop+41),a
    ld (WriteOffsetLoop+54),a
    ld (WriteOffsetLoop+67),a
    ld (WriteOffsetLoop+80),a
    ld (WriteOffsetLoop+93),a
    ret
    ELSE
    ld b,0
WriteLoop:
WriteRepeat = PIO_WRITE_UNALIGNED_UNROLL
    IF PIO_UNALIGNED_MODE == 1
WriteRepeat = PIO_UNALIGNED_UNROLL
    ENDIF
    IF PIO_UNALIGNED_MODE == 3
WriteRepeat = 4
    ENDIF
    DUP WriteRepeat
    ld a,b
    ld (IO_PROG_B),a
    ld a,(de)
    ld (IO_PROG_A),a
    ld (hl),WRITE_0
    ld (hl),WRITE_1
    inc de
    inc b
    EDUP
    jp nz,WriteLoop
    ENDIF
WriteDone:
    IF PIO_UNALIGNED_MODE == 3
WriteAlignedLoop = WriteLoop
    ENDIF
