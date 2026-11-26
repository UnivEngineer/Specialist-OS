;----------------------------------------------------------------------------

; Сохранить координаты найденной кнопки для повторного опроса.
; Вход HL — маска столбца, C — обратный номер строки, DE — адрес кода столбца.
; Выход B — код кнопки, keyColumn/keyRow задают координаты, AF/DE/HL сохранены.
keybScan3:
            ld      (keyColumn),hl
            push    de
            push    af
            ld      a,c
            ld      (keyRow),a
            ld      a, 6    ; e += (6 - c) * 16
            sub     c
            add     a, a
            add     a, a
            add     a, a
            add     a, a
            add     a, e
            ld      e, a
            ld      a, (de) ; b = *de;
            ld      b, a
            pop     af
            pop     de
            ret

