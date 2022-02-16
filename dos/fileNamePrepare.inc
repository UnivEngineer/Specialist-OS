;+---------------------------------------------------------------------------
; MXOS
; Подготовить имя файла для функций ОС
;
; На входе 
;  hl - исходное имя
;  de - буфер для результата (P_INPUT_WIDTH + 3 байт)
;
; 2013-12-12 Дизассемблировано vinxru
;----------------------------------------------------------------------------

fileNamePrepare:
            ; Сохраняем регистры
            push    bc
            push    de

            ; Если второй символ исходной строки ':', то меняем диск
            inc     hl          
            ld      a, (hl)
            cp      ':'
            dec     hl
            jp nz,  fileNamePr_1

            ; Меняем диск
            ld      a, (hl)       
            sub     'A'
            push    hl
            call    fileSelectDrive
            pop     hl

            ; Диск не входит в имя файла
            inc     hl          
            inc     hl          
            
fileNamePr_1:
            ; b = длина имени, c = 3 - длина расширения
            ld      bc, (DIR_NAME_LENGTH << 8) + 3

            ; Копируем текст до точки, пробела или конца строки, не более DIR_NAME_LENGTH символов.
fileNamePr_2:
            ld      a, (hl)
            and     a
            jp p,   fileNamePr_3
            sub     40h         ; проеобразование KOI?
fileNamePr_3:
            cp      ' '+1
            jp c,   fileNamePr_5
            inc     hl
            cp      '.'
            jp z,   fileNamePr_5            
            ld      (de), a
            inc     de
            dec     b
            jp nz,  fileNamePr_2

            ; Пропускаем текст до точки, пробела или конца строки
fileNamePr_4:
            ld      a, (hl)
            cp      ' '+1
            jp c,   fileNamePr_6
            inc     hl
            cp      '.'
            jp nz,  fileNamePr_4
            jp      fileNamePr_6

; ---------------------------------------------------------------------------

fileNamePr_5:
            ; Имя короче DIR_NAME_LENGTH символов
            ; Заполняем недостающие символы имени пробелами
            call    memset_de_20_b

            ; Копируем текст до пробела или конца строки, не более 3 символов. 
fileNamePr_6:
            ld      a, (hl)                         
            and     a
            jp p,   fileNamePr_7
            sub     40h         ; проеобразование KOI?
fileNamePr_7:
            cp      ' '+1
            jp c,   fileNamePr_11
            ld      (de),a
            inc     hl
            inc     de
            dec     c
            jp nz,  fileNamePr_6

fileNamePr_8:
            ; Пропускаем текст до пробела или конца строки
            ld      a, (hl)
            cp      ' '
            jp c,   fileNamePr_10
            inc     hl
            jp nz,  fileNamePr_8
            
;fileNamePr_9:
;            ; Пропускаем текст до пробела или конца строки
;            ; Почему то дублируется код
;            ld      a, (hl)
;            cp      ' '
;            jp c,   fileNamePr_10
;            inc     hl
;            jp z,   fileNamePr_9
;            dec     hl

fileNamePr_10:    ; Восстанавливаем регистры и выходим
            pop     de
            pop     bc
            ret

; ---------------------------------------------------------------------------

fileNamePr_11:
            ; Заполняем недостающие символы расширения пробелами
            ld      b, c
            call    memset_de_20_b

            ; Восстанавливаем регистры и выходим
            pop     de
            pop     bc
            ret

