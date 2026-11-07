; ABI: HL = список объектов, A = смещение X (0..47 байт).
; Бит 7 команды запрещает смещение. Разрушает AF/BC/DE/HL и флаги.
; Нереентерабельно: смещение хранится в операнде ADD.
;----------------------------------------------------------------------------
; MXOS DOS: графические окна
; Рисование окошек
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

uiDraw:
            ; A — смещение относительных объектов в байтах экрана.
            ld    (uiDrawOffset+1), a
uiDrawLoop:       ; Читаем 5 байт из hl в a, hl, c, b. 
            ; При этом de = hl+5
            ld    a, (hl)
            inc   hl
            ld    e, (hl)
            inc   hl
            ld    d, (hl)
            inc   hl
            ld    c, (hl)
            inc   hl
            ld    b, (hl)
            inc   hl          
            ex    de, hl

            ; Если a=0 (первый байт), то выйти
            and   a
            ret z 

            ; Если бит 7 сброшен, прибавляем явное смещение X из ABI.
            jp M, uiDrawAbsolute
            push  af
            ld    a, h
uiDrawOffset:
            add   0
            ld    h, a
            pop   af
uiDrawAbsolute:

            ; Если a&0xF==3, то заполнить c байт памяти байтом b
            and   0Fh
            cp    3
            jp z, uiDrawFill

            ; Если a&0xF==1, то нарисовать веритикальную линию
            ; b-первый байт, c-средняя длина, *de++ - последний байт 
            cp    1
            jp z, uiDrawLine

            ; Нарисовать окно
            call  uiDrawWindow

uiDrawNext:  ; hl=de и следующий цикл
            ex    de, hl
            jp    uiDrawLoop

; ---------------------------------------------------------------------------
; Заполнить c байт памяти байтом b

uiDrawFill:      ld    a, b
            call  uiFillColumn
            jp    uiDrawNext

; ---------------------------------------------------------------------------
; Линия hl-адрес, b-первый байт, c-средняя длина, *de++ - последний байт 

uiDrawLine:      ; *hl++ |= b
            ld    a, b
            or    (hl)
            ld    (hl), a
            inc   h
            
uiDrawLineLoop:
            ld    (hl), 0FFh
            inc   h
            dec   c
            jp nz, uiDrawLineLoop

            ; hl |= *de++
            ld    a, (de)
            inc   de
            or    (hl)
            ld    (hl), a

            jp    uiDrawNext

; ---------------------------------------------------------------------------
; Рисуем окно (b=ширина-2, с=высота-6)

uiDrawWindow:
            ; Если h - 90h + b >= 0x2F, то есть X + b + 2 > ширина экрана, то выйти
            ld    a, h
            sub   90h
            add   a, b
            cp    2Fh
            ret nc      

            ; Сохранить регистры
            push  hl
            push  bc
            push  de

            ; Линия
            ld    de, uiWindowPattern
            call  uiDrawColumn

            ; Повтор b раз
uiDrawWindow_2:
            push  de
            call  uiDrawColumn
            dec   b
            jp z, uiDrawWindow_3
            pop   de          
            jp    uiDrawWindow_2

; ---------------------------------------------------------------------------

uiDrawWindow_3:
            pop   af

            ; Линия
            call  uiDrawColumn

            ; Восстановить регистры и выйти
            pop   de
            pop   bc
            pop   hl
            ret

; ---------------------------------------------------------------------------
; Вертикальная линия. Задается три верхних байта, заполнитель и три нижних

uiDrawColumn:    ; Сохраняем регистры
            push  bc
            push  hl

            ; Копируем 3 байта (сверху-вниз)
            call  uiCopyThree

            ; 4 ый байт повторяем С-раз (сверху-вниз)
            ld    a, (de)
            call  uiFillColumn
            inc   de

            ; Копируем 3 байта (сверху-вниз)
            call  uiCopyThree
                  
            ; Восстаналиваем регистры
            pop   hl
            pop   bc

            ; Вправо
            inc   h
            ret


;----------------------------------------------------------------------------
; MXOS DOS
; Копирвоание 3-х байт из адреса de в адрес hl
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

uiCopyThree:
        ld    a, (de)
        ld    (hl), a
        inc   hl
        inc   de

        ld    a, (de)
        ld    (hl), a
        inc   hl
        inc   de

        ld    a, (de)
        ld    (hl), a
        inc   hl
        inc   de

        ret

;----------------------------------------------------------------------------
; MXOS DOS
; Заполнение памяти по адресу hl байтом a длинной c
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

uiFillColumn:
        ld      (hl), a
        inc     hl
        dec     c
        jp nz,  uiFillColumn
        ret

uiWindowPattern:   DB 01111111b            ; Верхний левый угол
            DB 01000000b
            DB 01011111b
            DB 01010000b            ; Левый край
            DB 01011111b            ; Нижний левый угол
            DB 01000000b
            DB 01111111b

            DB 11111111b            ; Верх окна
            DB 00000000b
            DB 11111111b

            DB 00000000b            ; Содержимое окна

            DB 11111111b            ; Низ окна
            DB 00000000b
            DB 11111111b

            DB 11111110b            ; Правый верхний угол
            DB 00000010b
            DB 11111010b
            DB 00001010b            ; Правая граница
            DB 11111010b            ; Правый нижний угол
            DB 00000010b
            DB 11111110b

