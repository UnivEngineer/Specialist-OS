; Перед общим memmove очищаем отбрасываемые 10 строк всех полос.
; Тогда переход через границу полосы переносит фон, а не чужие символы.
; A=Y отбрасываемой области (0 вверх, 246 вниз); AF/BC вызывающего в стеке.
scrollUp2:
scrollCopyFill:
            push af
            push hl
            push de
            push bc
            call scrollFillAll
            pop bc
            pop de
            pop hl
            call bios_memcpy_bc_hl
            pop af
            ; Край, не затронутый переносом: BFF6..BFFF вверх, 9000..9009 вниз.
            ld h,090h
            or a
            jp nz,scrollFinalBand
            ld h,0BFh
scrollFinalBand:
            xor 246
            ld l,a
            call scrollFillTen
            pop af
            pop bc
            ret
scrollFillAll:
            ld l,a
            ld h,090h
            ld b,48
scrollFillColumn:
            push hl
            call scrollFillTen
            pop hl
            inc h
            dec b
            jp nz,scrollFillColumn
            ret
scrollFillTen:
            push hl
            ld hl,(bios_vars.inverse)
            ex de,hl
            pop hl
            DUP 5
            ld (hl),e
            inc l
            ld (hl),d
            inc l
            EDUP
            ret
