; Общий путь: один memmove битмапа и очистка десяти строк.
; A = Y очищаемой области (0 или 246). AF/BC вызывающего лежат в стеке.
; Переносы через границы столбцов закрываются заливкой.
; Всего 12278 чтений и 12758 записей экрана.
scrollUp2:
scrollCopyFill:
            push af
            call bios_memcpy_bc_hl
            pop af
            ld hl, (bios_vars.inverse)
            ex de, hl
            ld l, a
            ld h, 090h
            ld b, 48
scrollFillColumn:
            push hl
            DUP 5
            ld (hl), e
            inc l
            ld (hl), d
            inc l
            EDUP
            pop hl
            inc h
            dec b
            jp nz, scrollFillColumn
            pop af
            pop bc
            ret
