; Локальная UI-библиотека для программы MAIN или DLL MAIN.
; Сначала INCLUDE для макросов. В секции кода DEFINE UI_IMPLEMENTATION
; и повторный INCLUDE включают процедуры и рабочие данные ровно один раз.
    IFNDEF UI_DEFINITIONS
    DEFINE UI_DEFINITIONS
; Формат списков объектов uiDraw. Координаты в пикселях.
; A: бит 7 — абсолютный X; иначе добавляется смещение из ABI-входа A.
; Список заканчивается одним нулевым байтом.
    MACRO G_WINDOW A,X,Y,W,H
        DB A|2, Y, 90h+(X>>3), H-6, (W>>3)-2
    ENDM
    MACRO G_LINE A,X,Y,W
        DB A|1, Y, 90h+(X>>3), (((X&7)+W+7)>>3)-2, 0FFh>>(X&7), (0FF00h>>((W+X)&7)) & 0FFh
    ENDM
    MACRO G_VLINE A,X,Y,H
        DB A|3, Y, 90h+(X>>3), H, 80h>>(X&7)
    ENDM

; Поле тулбара: ширина 1..4 экранных байта, ASCII-текст до нуля.
; F=0 — клавиша, F=80h — инверсная подпись; сумма ширин строки — 48.
    MACRO UI_TOOLBAR_FIELD W,F,TEXT
        ASSERT W >= 1 && W <= 4
        ASSERT F == 0 || F == 080h
        DB W|F,TEXT,0
    ENDM

    ENDIF

    IFDEF UI_IMPLEMENTATION
    IFNDEF UI_CODE_INCLUDED
    DEFINE UI_CODE_INCLUDED

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


; ABI: HL = экранный адрес, B = ширина в байтах, C = высота (1..255).
; Размеры ненулевые; область полностью внутри экрана.
; DE сохраняется; AF/BC/HL и флаги разрушаются. Цвет задаёт вызывающий.
uiInverseRect:
            push    de
            ld      e,c
uiInverseLoop_x:
            ld      c,e
            push    hl
uiInverseLoop_y:
            ld      a,(hl)
            cpl
            ld      (hl),a
            inc     hl
            dec     c
            jp nz,  uiInverseLoop_y
            pop     hl
            inc     h
            dec     b
            jp nz,  uiInverseLoop_x
            pop     de
            ret

; HL=список полей [ширина в байтах | bit7 подписи, ASCIIZ], 0 завершает.
; B/C=цвет клавиш/подписей. Поле 1..4 байта, ASCII-текст помещается целиком.
; Высота 9, Y=247..255. Координаты и инверсия BIOS не меняются.
; AF/BC/DE/HL разрушены; нереентерабельно, как прочие графические службы.
; Каждый экранный байт сразу получает окончательные битмап и атрибут.
uiToolbar:
            ld a,b
            ld (toolbarKeyColor),a
            ld a,c
            ld (toolbarLabelColor),a
            ld a,090h
            ld (toolbarColumn),a
uiToolbarField:
            ld a,(hl)
            or a
            ret z
            inc hl
            ld (toolbarText),hl
            ld c,a
            and 7
            ld (toolbarWidth),a
            ld a,c
            rlca
            sbc a,a
            ld (toolbarInverse),a
            ld a,(toolbarKeyColor)
            jp nc,uiToolbarColorReady
            ld a,(toolbarLabelColor)
uiToolbarColorReady:
            ld (toolbarColor),a
            ld b,0
uiToolbarMeasure:
            ld a,(hl)
            inc hl
            or a
            jp z,uiToolbarMeasured
            inc b
            jp uiToolbarMeasure
uiToolbarMeasured:
            ld (toolbarNext),hl
            ld a,b
            add a,a
            add a,b
            add a,a
            ld c,a
            ld a,(toolbarWidth)
            add a,a
            add a,a
            add a,a
            sub c
            rrca
            ld (toolbarPixel),a
            ; Очищается только малый внеэкранный буфер одной ячейки.
            ld hl,toolbarBitmap
            ld b,37
            xor a
uiToolbarZero:
            ld (hl),a
            inc hl
            dec b
            jp nz,uiToolbarZero
uiToolbarCharacter:
            ld hl,(toolbarText)
            ld a,(hl)
            or a
            jp z,uiToolbarCopy
            inc hl
            ld (toolbarText),hl
            ; Полный штатный шрифт: код — младший байт, строка — старший.
            ld l,a
            ld h,FONT_ADDR >> 8
            push hl
            ld a,(toolbarPixel)
            ld c,a
            and 7
            ld (toolbarShift),a
            ld a,c
            rrca
            rrca
            rrca
            and 3
            ld e,a
            ld d,0
            ld hl,toolbarBitmap+4 ; Верхняя строка — отступ над глифом.
            add hl,de
            ex de,hl
            pop hl
            ld b,8
uiToolbarGlyphRow:
            push bc
            push de
            ld a,(hl)
            and 01Fh
            add a,a
            add a,a
            add a,a
            ld b,a
            ld c,0
            ld a,(toolbarShift)
            ld e,a
            or a
            jp z,uiToolbarShifted
uiToolbarShiftRow:
            ld a,b
            or a
            rra
            ld b,a
            ld a,c
            rra
            ld c,a
            dec e
            jp nz,uiToolbarShiftRow
uiToolbarShifted:
            pop de
            ld a,(de)
            or b
            ld (de),a
            inc de
            ld a,(de)
            or c
            ld (de),a
            inc de
            inc de
            inc de
            inc h
            pop bc
            dec b
            jp nz,uiToolbarGlyphRow
            ld a,(toolbarPixel)
            add a,6
            ld (toolbarPixel),a
            jp uiToolbarCharacter
uiToolbarCopy:
            ld a,(toolbarInverse)
            ld (uiToolbarXor+1),a
            ld a,(toolbarColor)
    IF ENABLE_COLOR
            ld (IO_COLOR),a
    ENDIF
            ld a,(toolbarColumn)
            ld h,a
            ld de,toolbarBitmap
            ld a,(toolbarWidth)
            ld b,a
uiToolbarCopyColumn:
            push de
            ld l,247
            ld c,9
uiToolbarCopyRow:
            ld a,(de)
uiToolbarXor:
            xor 0
uiToolbarWrite:
            ld (hl),a
            inc de
            inc de
            inc de
            inc de
            inc l
            dec c
            jp nz,uiToolbarCopyRow
            pop de
            inc de
            inc h
            dec b
            jp nz,uiToolbarCopyColumn
            ld a,h
            ld (toolbarColumn),a
            ld hl,(toolbarNext)
            jp uiToolbarField

toolbarText: DW 0
toolbarNext: DW 0
toolbarKeyColor: DB 0
toolbarLabelColor: DB 0
toolbarColor: DB 0
toolbarColumn: DB 0
toolbarWidth: DB 0
toolbarInverse: DB 0
toolbarPixel: DB 0
toolbarShift: DB 0
; 4 столбца * 9 строк; дополнительный байт принимает нулевой хвост глифа.
toolbarBitmap: DS 37,0

; Копирование прямоугольника битмапа, без переноса цветовых атрибутов.
; HL = источник, BC = назначение; D = ширина в байтах, E = высота.
; Ширина/высота ненулевые; оба прямоугольника внутри экрана, Y+E <= 256.
; Столбцы обрабатываются слева направо; вертикальное перекрытие допустимо.
; При горизонтальном перекрытии назначение должно быть левее источника.
; Выход: HL/BC смещены на D*256, D=0, E сохранён; AF/флаги разрушены.
; Цвет задаёт вызывающий, для прокрутки вся область должна иметь один атрибут.
; Код 22 байта, без буфера. На столбец 148 тактов обвязки + memcpy;
; D*E чтений и столько же записей экрана, без обращений к портам.
; Стенд 8080: 10x170 вверх ~51,8 тыс. тактов, вниз ~53,0 тыс.;
; максимальный проверенный 48x255 — ~366 тыс. тактов (с обвязкой стенда).
uiCopyRect:
            push    de
            push    bc
            push    hl
            push    hl
            ld      d, 0
            add     hl, de
            ex      de, hl
            pop     hl
            call    bios_memcpy_bc_hl
            pop     hl
            pop     bc
            pop     de
            inc     h
            inc     b
            dec     d
            jp nz,  uiCopyRect
            ret

    ENDIF
    ENDIF
