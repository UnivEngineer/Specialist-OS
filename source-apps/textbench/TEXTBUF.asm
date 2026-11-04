            include "bufferabi.inc"
            org 0100h
entry:      di
start:      ld hl, text
nextBatch:  push de
            ld de, BUFFER_ADDR
            ld b, 0
fill:       ld a, (hl)
            or a
            jp z, flush
            ld (de), a
            inc de
            inc hl
            inc b
            ld a, b
            cp BUFFER_SIZE
            jp nz, fill
flush:      ld a, b
            or a
            jp z, nextLine
            ld c, b
            ld b, 1
            xor a
output:     call BUFFER_GATE
            pop de
            jp nextBatch
nextLine:   pop de
            dec de
            ld a, d
            or e
            jp nz, start
done:       jp done
text:       defs 50, 0
