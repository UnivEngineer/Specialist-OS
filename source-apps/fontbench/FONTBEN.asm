            include "api.inc"
            org 0100h
; rowsLeft и corpus заполняет стенд до измерения. Стек 8000h.
; В обоих размещениях меняются только два операнда CALL.
entry:      di
            ld hl, 09000h
            ld de, corpus
next:
            IF BATCH == 0
            ld a, (de)
            or a
            jp z, endRow
            ld c, a
            ld b, 0
            xor a
output:     call BUFFER_GATE
            inc de
            inc h
            jp next
            ELSE
            push hl
            ld hl, BUFFER_ADDR
            ld b, 0
fill:       ld a, (de)
            or a
            jp z, flush
            ld (hl), a
            inc de
            inc l
            inc b
            ld a, b
            cp BUFFER_SIZE
            jp nz, fill
flush:      pop hl
            ld a, b
            or a
            jp z, endRow
            ld c, b
            ld b, 1
            xor a
output:     call BUFFER_GATE
            ld a, h
            add a, c
            ld h, a
            jp next
            ENDIF
endRow:     push hl
            ld hl, (rowsLeft)
            dec hl
            ld (rowsLeft), hl
            ld a, h
            or l
            pop hl
            jp z, done
            inc de
            ld a, l
            add a, 8
            ld l, a
            ld h, 090h
            jp nc, next
            ld l, 248
            ld b, 2
            xor a
scrollOutput:
            call BUFFER_GATE
            jp next
done:       jp done
rowsLeft:   dw 0
corpus:     defs 4900, 0
            assert $ < FONT8_ADDR
