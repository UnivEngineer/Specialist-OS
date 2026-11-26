; Локальная UI-библиотека для программы MAIN или DLL MAIN.
; Сначала INCLUDE для макросов. В секции кода DEFINE UI_IMPLEMENTATION
; и повторный INCLUDE включают процедуры и рабочие данные ровно один раз.
; DEFINE UI_PIXEL_RECT adds uiFillRectPixels/uiInverseRectPixels/uiFrameRectPixels; see
; ui-pixel-rect.inc for pixel coordinates, masks and register contracts.
; DEFINE UI_BYTE_RECT adds uiFillRectBytes/uiFrameRectBytes/uiRecolorRectBytes. Existing uiDraw,
; uiInverseRect and uiCopyRect remain the fast byte-aligned window path.
; DEFINE UI_TEXT_RECT adds uiCopyTextRect for 8-of-10 text-cell scrolling.
; DEFINE UI_TEXT_RECT_COLORS additionally enables a per-row palette copy;
; requires UI_TEXT_RECT. Descriptor and stack/overlap contracts: UI.md.
; DEFINE UI_BITMAP adds raw and software-palette bitmap save/restore.
; UI_MODAL_BEGIN/BUTTON/ROW/END declare shared button dialogs; include
; ui-modal.inc after ui-text-align.inc and ui-button.inc for their rendering.
; All public contracts and examples: UI.md. No application state is required.
    IFNDEF UI_DEFINITIONS
    DEFINE UI_DEFINITIONS
    INCLUDE "../hardware/spec_mx.inc"
    INCLUDE "../formats/font.inc"
; Формат списков объектов uiDraw. Координаты в пикселях.
; A: бит 7 — абсолютный X; иначе добавляется смещение из ABI-входа A.
; Список заканчивается одним нулевым байтом.
    MACRO G_WINDOW A,X,Y,W,H
        DB A|2, Y, 90h+(X>>3), H-6, (W>>3)-2
    ENDM
    MACRO G_MODAL_WINDOW A,X,Y,W,H
        DB A|12h, Y, 90h+(X>>3), H-6, (W>>3)-2
    ENDM
    STRUCT UI_SCROLLBAR
x               DW 0
y               DB 0
length          DB 0
thickness       DB 0
horizontal      DB 0
total           DW 0
visible         DW 0
position        DW 0
    ENDS
uiNoticeTop    = 112
uiNoticeHeight = 32
    MACRO UI_MODAL_NOTICE
        DB 131,120,0,0,0
        DW 0
        G_MODAL_WINDOW 80h,128,uiNoticeTop,128,uiNoticeHeight
        DB 0
    ENDM
    STRUCT UI_BUTTON
text            DW 0
left            DB 0    ; X/2
width           DB 0    ; Width/2
top             DB 0
height          DB 14
font            DB 6
selected        DB 0
normal          DB 007h
focused         DB 003h
    ENDS
    MACRO G_LINE A,X,Y,W
        DB A|1, Y, 90h+(X>>3), (((X&7)+W+7)>>3)-2, 0FFh>>(X&7), (0FF00h>>((W+X)&7)) & 0FFh
    ENDM
    MACRO G_VLINE A,X,Y,H
        DB A|3, Y, 90h+(X>>3), H, 80h>>(X&7)
    ENDM

; Shared centered modal style. BEGIN, row-major BUTTONs, END: clients specify
; body height and a grid only. Coordinates, spacing and borders are private.
    MACRO UI_MODAL_BEGIN argBodyHeight,argRowCount,argColCount
    DEFINE UI_M_LEFT 64
    DEFINE UI_M_WIDTH 256
    DEFINE UI_M_COMPACT 0
    UI_MODAL_CORE argBodyHeight,argRowCount,argColCount,1
    ENDM
    MACRO UI_MODAL_COMPACT_BEGIN argBodyHeight,argRowCount,argColCount
    DEFINE UI_M_LEFT 24
    DEFINE UI_M_WIDTH 144
    DEFINE UI_M_COMPACT 1
    UI_MODAL_CORE argBodyHeight,argRowCount,argColCount,1
    ENDM
    MACRO UI_MODAL_BUTTONS_BEGIN argRowCount,argColCount
    DEFINE UI_M_LEFT 64
    DEFINE UI_M_WIDTH 256
    DEFINE UI_M_COMPACT 0
    UI_MODAL_CORE 0,argRowCount,argColCount,0
    ENDM
    MACRO UI_MODAL_COMPACT_BUTTONS_BEGIN argRowCount,argColCount
    DEFINE UI_M_LEFT 24
    DEFINE UI_M_WIDTH 144
    DEFINE UI_M_COMPACT 1
    UI_MODAL_CORE 0,argRowCount,argColCount,0
    ENDM
    MACRO UI_MODAL_CORE argBodyHeight,argRowCount,argColCount,argHasBody
    ASSERT argColCount >= 1 && argColCount <= 4 && argRowCount >= 1
    DEFINE UI_M_BUTTON_Y (26+argBodyHeight+9*argHasBody)
    DEFINE UI_M_ROWS argRowCount
    DEFINE UI_M_COLS argColCount
    DEFINE UI_M_GRID_COLS argColCount
    DEFINE UI_M_HEIGHT (UI_M_BUTTON_Y+5+18*argRowCount)
    ASSERT argBodyHeight >= 0 && UI_M_HEIGHT <= 256
    DEFINE UI_M_TOP ((256-UI_M_HEIGHT)/2)
    DB UI_M_TOP+16,UI_M_TOP+26,argBodyHeight,UI_M_TOP+UI_M_BUTTON_Y,argRowCount*argColCount
    DW $+14+6*argHasBody
    G_MODAL_WINDOW 80h,UI_M_LEFT,UI_M_TOP,UI_M_WIDTH,UI_M_HEIGHT
    G_LINE 80h,UI_M_LEFT+4,UI_M_TOP+21,UI_M_WIDTH-8
    IF argHasBody
    G_LINE 80h,UI_M_LEFT+4,UI_M_TOP+UI_M_BUTTON_Y-5,UI_M_WIDTH-8
    ENDIF
    DB 0
    ENDM
    MACRO UI_MODAL_BUTTON argCaption,argRowIndex,argColIndex,argSelected
    ASSERT argRowIndex < UI_M_ROWS && argColIndex < UI_M_COLS
    IF UI_M_COMPACT
    IF UI_M_GRID_COLS == 1
    UI_BUTTON argCaption,32,32,UI_M_TOP+UI_M_BUTTON_Y+18*argRowIndex,14,6,argSelected
    ELSE
    IF UI_M_GRID_COLS == 2
    UI_BUTTON argCaption,20+32*argColIndex+16*(2-UI_M_COLS),24,UI_M_TOP+UI_M_BUTTON_Y+18*argRowIndex,14,6,argSelected
    ELSE
    IF UI_M_GRID_COLS == 3
    IF UI_M_COLS == 2
    ; Center two 32px buttons on byte boundaries with a 16px gap.
    UI_BUTTON argCaption,28+24*argColIndex,16,UI_M_TOP+UI_M_BUTTON_Y+18*argRowIndex,14,6,argSelected
    ELSE
    UI_BUTTON argCaption,20+20*argColIndex+10*(3-UI_M_COLS),16,UI_M_TOP+UI_M_BUTTON_Y+18*argRowIndex,14,6,argSelected
    ENDIF
    ELSE
    UI_BUTTON argCaption,20+16*argColIndex+8*(4-UI_M_COLS),8,UI_M_TOP+UI_M_BUTTON_Y+18*argRowIndex,14,6,argSelected
    ENDIF
    ENDIF
    ENDIF
    ELSE
    IF UI_M_COLS == 1
    UI_BUTTON argCaption,68,56,UI_M_TOP+UI_M_BUTTON_Y+18*argRowIndex,14,6,argSelected
    ELSE
    IF UI_M_COLS == 2
    UI_BUTTON argCaption,36+64*argColIndex,56,UI_M_TOP+UI_M_BUTTON_Y+18*argRowIndex,14,6,argSelected
    ELSE
    IF UI_M_COLS == 3
    UI_BUTTON argCaption,40+40*argColIndex,32,UI_M_TOP+UI_M_BUTTON_Y+18*argRowIndex,14,6,argSelected
    ELSE
    UI_BUTTON argCaption,44+28*argColIndex,20,UI_M_TOP+UI_M_BUTTON_Y+18*argRowIndex,14,6,argSelected
    ENDIF
    ENDIF
    ENDIF
    ENDIF
    ENDM
    MACRO UI_MODAL_ROW argColumnCount
    ASSERT argColumnCount >= 1 && argColumnCount <= 4
    UNDEFINE UI_M_COLS
    DEFINE UI_M_COLS argColumnCount
    ENDM
    MACRO UI_MODAL_END
    UNDEFINE UI_M_BUTTON_Y
    UNDEFINE UI_M_ROWS
    UNDEFINE UI_M_COLS
    UNDEFINE UI_M_GRID_COLS
    UNDEFINE UI_M_HEIGHT
    UNDEFINE UI_M_TOP
    UNDEFINE UI_M_LEFT
    UNDEFINE UI_M_WIDTH
    UNDEFINE UI_M_COMPACT
    ENDM

; Поле тулбара: ширина 1..48 экранных байта, ASCII-текст до нуля.
; F=0 — клавиша, F=80h — инверсная подпись; сумма ширин строки — 48.
    MACRO UI_TOOLBAR_FIELD W,F,TEXT
        ASSERT W >= 1 && W <= 48
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
    IFDEF UI_MODAL_BORDER
            ld (uiDrawKind),a
    ENDIF

            ; Если a&0xF==3, то заполнить c байт памяти байтом b
            and   0Fh
            cp    3
            jp z, uiDrawFill

            ; Если a&0xF==1, то нарисовать веритикальную линию
            ; b-первый байт, c-средняя длина, *de++ - последний байт
            cp    1
            jp z, uiDrawLine

            ; Нарисовать окно
    IFDEF UI_MODAL_BORDER
            ld a,(uiDrawKind)
            and 10h
            jp z,uiDrawPlainWindow
            call uiDrawModalWindow
            jp uiDrawNext
uiDrawPlainWindow:
    ENDIF
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

    IFDEF UI_MODAL_BORDER
uiDrawKind:         DB 0
uiWindowPadding:    DB 0
uiDrawModalWindow:
            ld a,2
            jp uiDrawWindowPadding
    ENDIF
uiDrawWindow:
    IFDEF UI_MODAL_BORDER
            xor a
uiDrawWindowPadding:
            ld (uiWindowPadding),a
    ENDIF
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
    IFDEF UI_MODAL_BORDER
            ld a,(uiWindowPadding)
            add a,a
            ld e,a
            ld a,c
            sub e
            ld c,a
    ENDIF

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
    IFDEF UI_MODAL_BORDER
            call uiWindowPad
    ENDIF
            call  uiCopyThree

            ; 4 ый байт повторяем С-раз (сверху-вниз)
            ld    a, (de)
            call  uiFillColumn
            inc   de

            ; Копируем 3 байта (сверху-вниз)
            call  uiCopyThree
    IFDEF UI_MODAL_BORDER
            call uiWindowPad
    ENDIF

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

    IFDEF UI_MODAL_BORDER
uiWindowPad:
        ld a,(uiWindowPadding)
        or a
        ret z
        ld (hl),0
        inc l
        ld (hl),0
        inc l
        ret
    ENDIF
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
    IFNDEF UI_NO_TOOLBAR
uiToolbar:
    ld a,b
    ld (toolbarNativeKeyColor),a
    ld a,c
    ld (toolbarNativeLabelColor),a
    ex de,hl
    ld hl,(bios_vars.cursorY)
    push hl
    ld hl,(bios_vars.inverse)
    push hl
    ex de,hl
    ld a,090h
    ld (toolbarNativeColumn),a
toolbarNativeField:
    ld a,(hl)
    inc hl
    or a
    jp z,toolbarNativeCapture
    ld (toolbarNativeFieldSpec),a
    and 7Fh
    ld (toolbarNativeFieldWidth),a
    ld b,a
    push hl
    ; Fill complete field with the final background and attribute.
    call toolbarNativeColor
    ld a,(toolbarNativeColumn)
    ld h,a
toolbarNativeFillColumn:
    ld l,247
    ld c,9
    ld a,(bios_vars.inverse)
toolbarNativeFillRow:
    ld (hl),a
    inc l
    dec c
    jp nz,toolbarNativeFillRow
    inc h
    dec b
    jp nz,toolbarNativeFillColumn
    pop hl
    push hl
    ld b,0
toolbarNativeLength:
    ld a,(hl)
    or a
    jp z,toolbarNativeMeasured
    inc b
    inc hl
    jp toolbarNativeLength
toolbarNativeMeasured:
    ld a,b
    call toolbarNativeAdvance
    ld c,a
    ld a,(toolbarNativeFieldWidth)
    add a,a
    add a,a
    sub c
    rra
    ld c,a
    ld a,(toolbarNativeColumn)
    sub 90h
    add a,a
    add a,a
    add a,c
    ; Snap origin to the engine's 4/8-pixel cell; 6px accepts any X.
    ld h,a
    ld a,(FONT_ENGINE_HEADER+5)
    cp 6
    ld a,h
    jp z,toolbarNativeXReady
    and 0FEh
    ld h,a
    ld a,(FONT_ENGINE_HEADER+5)
    cp 4
    ld a,h
    jp z,toolbarNativeXReady
    and 0FCh
toolbarNativeXReady:
    ld h,a
    ld l,255
    ld (bios_vars.cursorY),hl
    pop hl
    ; A toolbar is a bitmap row, not a console stream. The FNT1 scalar entry
    ; draws a glyph and advances X without the console's post-glyph newline.
    ; In particular a full final field at Y=255 must never scroll the screen.
    call toolbarNativeText
    inc hl
    ld a,(toolbarNativeFieldWidth)
    ld b,a
    ld a,(toolbarNativeColumn)
    add a,b
    ld (toolbarNativeColumn),a
    jp toolbarNativeField
toolbarNativeColor:
    ld a,(toolbarNativeFieldSpec)
    and 80h
    ld a,(toolbarNativeKeyColor)
    ld de,0
    jp z,toolbarNativeSetColor
    ld a,(toolbarNativeLabelColor)
    dec de
toolbarNativeSetColor:
    ld (IO_COLOR),a
    push hl
    ex de,hl
    ld (bios_vars.inverse),hl
    pop hl
    ret
toolbarNativeCapture:
    pop hl
    ld (bios_vars.inverse),hl
    pop hl
    ld (bios_vars.cursorY),hl
    ret
toolbarNativeText:
    ld a,(hl)
    or a
    ret z
    ld c,a
    push hl
    ld hl,(bios_upper.charGen)
    call FONT_ENGINE_SCALAR
    pop hl
    inc hl
    jp toolbarNativeText
toolbarNativeAdvance:
    ld b,a
    ld a,(FONT_ENGINE_HEADER+5)
    or a
    rra
    ld c,a
    xor a
    inc b
toolbarNativeMultiply:
    dec b
    ret z
    add a,c
    jp toolbarNativeMultiply
toolbarNativeColumn:        DB 0
toolbarNativeFieldSpec:     DB 0
toolbarNativeFieldWidth:    DB 0
toolbarNativeKeyColor:      DB 0
toolbarNativeLabelColor:    DB 0



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
    ENDIF ; UI_NO_TOOLBAR

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

    IFDEF UI_PIXEL_RECT
        INCLUDE "ui-pixel-rect.inc"
    ENDIF
    IFDEF UI_BYTE_RECT
        INCLUDE "ui-byte-rect.inc"
    ENDIF
    IFDEF UI_BITMAP
        INCLUDE "ui-bitmap.inc"
    ENDIF
    IFDEF UI_TEXT_RECT
        INCLUDE "ui-text-rect.inc"
    ENDIF
    IFDEF UI_TEXT_RECT_COLORS
        INCLUDE "ui-text-colors.inc"
    ENDIF
    IFDEF UI_SCROLL_BAR
        INCLUDE "ui-scrollbar.inc"
    ENDIF

    ENDIF
    ENDIF
