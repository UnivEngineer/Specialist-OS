            include "api.inc"
            org 0100h
entry:      di
row:        ld hl, text
next:
            IF BATCH == 0
            ld a, (hl)
            or a
            jp z, endRow
            ld c, a
            ld b, 0
            xor a
output:     call BUFFER_GATE
            inc hl
            jp next
            ELSE
            push de
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
            jp z, endBufferRow
            ld c, b
            ld b, 1
            xor a
output:     call BUFFER_GATE
            pop de
            jp next
endBufferRow:
            pop de
            ENDIF
endRow:     dec de
            ld a, d
            or e
            jp z, done
            ld b, 2
            xor a
lineOutput: call BUFFER_GATE
            jp row
done:       jp done
text:       defs 49, 0
