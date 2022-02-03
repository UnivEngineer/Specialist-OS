;----------------------------------------------------------------------------
; MXOS NC.COM
; Убрать из имени пробелы
;
; вход:
;   hl - откуда
;   de - куда
;
; сохраняемые регистры:
;   hl, bc
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

compactName:
            ; Сохраняем регистры
            push    hl
            push    bc

            ; Копируем до пробела, но не более DIR_NAME_LENGTH букв
            ld      bc, DIR_NAME_LENGTH
compactName_1:      ld  a, (hl)
            cp      ' '
            jp z,   compactName_2           
            ld      (de),a
            inc     hl
            inc     de
            dec     c
            jp nz,  compactName_1

compactName_2:
            ; Пропущенные символы исходной строки
            add     hl, bc

            ; Точка
            ld      a, '.'
            ld      (de),a
            inc     de

            ; Копируем до пробела, но не более 3 букв
            ld      c, 3
compactName_3:
            ld      a, (hl)
            cp      ' '
            jp z,   compactName_4
            ld      (de),a
            inc     hl
            inc     de
            dec     c
            jp nz,  compactName_3

compactName_4:
            ex      de, hl
            push    af

            ; Если последний символ '.', то удаляем его
            dec     hl
            ld      a, (hl)
            cp      '.'
            jp z,   compactName_5
            inc     hl

compactName_5:
            ; Добавляем пробел и конец строки
            ld      (hl), ' '
            inc     hl
            ld      (hl), 0Dh
            inc     hl
            ld      (hl), 0

            ; Результат в de
            ex    de, hl

            ; Восстаналиваем регистры и выходим
            pop   af
            pop   bc
            pop   hl
            ret
