            include "../../source/include/mxos.inc"
            include "api.inc"
            org FONT8_RENDER
; C = код 0..255, HL = адрес ячейки 8x8; L кратен 8, H = 90h..BFh.
; Знакогенератор: 256 глифов по 8 байт, восемь строк сверху вниз.
; Непрозрачная запись всех восьми бит. Все регистры и флаги сохранены.
draw8:      push af
            push bc
            push de
            push hl
            ld d, h
            ld e, l
            ld h, 0
            ld l, c
            add hl, hl
            add hl, hl
            add hl, hl
            ld bc, FONT8_ADDR
            add hl, bc
; Младшие байты не переносят адрес внутри выровненного глифа/ячейки.
            DUP 7
            ld a, (hl)
            ld (de), a
            inc l
            inc e
            EDUP
            ld a, (hl)
            ld (de), a
            pop hl
            pop de
            pop bc
            pop af
            ret
; C = длина 0..20, HL = первая ячейка; строка не пересекает край экрана.
; Буфер общий, без терминатора. Все регистры и флаги сохранены.
buffer8:    push af
            push bc
            push de
            push hl
            ld a, c
            or a
            jp z, bufferDone
            cp BUFFER_SIZE+1
            jp nc, bufferDone
            ld b, c
            ld de, BUFFER_ADDR
bufferLoop: ld a, (de)
            ld c, a
            call draw8
            inc de
            inc h
            dec b
            jp nz, bufferLoop
bufferDone: pop hl
            pop de
            pop bc
            pop af
            ret
; Прокрутка на восемь пикселей, освобождённые строки обнуляются.
; Цветовые атрибуты едины и не меняются. Все регистры/флаги сохранены.
scroll8:    push af
            push bc
            push de
            push hl
            ld hl, 09008h
            ld de, 0C000h
            ld bc, 09000h
            call bios_memcpy_bc_hl
            xor a
            ld hl, 090F8h
            ld b, 48
clearColumn:
            DUP 8
            ld (hl), a
            inc l
            EDUP
            inc h
            ld l, 248
            dec b
            jp nz, clearColumn
            pop hl
            pop de
            pop bc
            pop af
            ret
            assert $ <= 0F000h
