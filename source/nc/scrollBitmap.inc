; Битмап-прокрутка двух колонок NC через штатный memcpy DOS.
; Вход B=1 вперёд, B=2 назад; vars.scrollRows=модуль шага.
; Обычно старый курсор стёрт. При scrollCursorKept его строка не копируется.
; Вне выделения атрибут всех строк одинаковый COLOR_FILE.
; CF=1: список и новый курсор нарисованы; AF/BC/DE/HL разрушены.
; CF=0: шаг больше колонки, нужен текстовый путь; BC/DE/HL сохранены.
; Межколоночный перенос делается до изменения его источника.
; При обычном шаге 1 копируется по 3500 байт чтения/записи экрана,
; при сохранённом курсоре — по 3400 и печатаются 2 имени вместо 36.
; Каталог берётся из кэша. Стенд 8080 без диска, обычный путь:
; 148–151 тыс. тактов на шаг 1 против 565 тыс.
; текстом; шаг 18 — 347–349 тыс. Большие шаги идут текстовым путём.

SCROLL_TOP EQU P_FILE_LIST_Y - 8
SCROLL_BOTTOM EQU P_FILE_LIST_Y_MAX - 8
SCROLL_HEIGHT EQU P_FILE_LIST_H * 10
SCROLL_LEFT EQU 90h + P_FILE_LIST_X1 * 2 / 8
SCROLL_RIGHT EQU 90h + P_FILE_LIST_X2 * 2 / 8

scrollBitmapPanel:
            ld      a, (vars.scrollRows)
            cp      P_FILE_LIST_H + 1
            ret nc
            push    bc
            ld      hl, state.panelA_drive
            call    readBytePanel
            call    loadFiles
            COLOR   COLOR_FILE
            ld      a, (vars.scrollRows)
            add     a, a
            ld      c, a
            add     a, a
            add     a, a
            add     a, c
            ld      (vars.scrollPixels), a
            ld      e, a
            ld      a, SCROLL_HEIGHT
            sub     e
            ld      (vars.scrollRemain), a
            pop     bc
            dec     b
            jp nz,  bitmapBackward

            ; Вперёд: левая вверх, верх правой в низ левой, правая вверх.
            ld      hl, (SCROLL_LEFT << 8) + SCROLL_TOP
            ld      bc, (SCROLL_LEFT << 8) + SCROLL_TOP
            ld      a, (vars.scrollPixels)
            add     a, l
            ld      l, a
            ld      a, (vars.scrollRemain)
            ld      e, a
            ld      d, 10
            call    bitmapCopy
            ld      hl, (SCROLL_RIGHT << 8) + SCROLL_TOP
            ld      a, (vars.scrollRemain)
            add     SCROLL_TOP
            ld      c, a
            ld      a, (vars.scrollPixels)
            ld      e, a
            call    bitmapCopy
            ld      h, SCROLL_RIGHT
            ld      b, SCROLL_RIGHT
            ld      c, SCROLL_TOP
            ld      a, (vars.scrollPixels)
            add     SCROLL_TOP
            ld      l, a
            ld      a, (vars.scrollRemain)
            ld      e, a
            ld      a, (vars.scrollCursorKept)
            or      a
            jp z,   bitmapRightCopy
            ; Последняя исходная строка содержит старую инверсию.
            ld      a, e
            sub     10
            ld      e, a
bitmapRightCopy:
            call    bitmapCopy

            ; Общий цикл печатает только новые строки.
            ld      a, (vars.scrollRows)
            ld      b, a
            call    printFilesDownUp
            scf
            ret

bitmapBackward:
            ; Назад: правая вниз, низ левой в верх правой, левая вниз.
            ld      hl, (SCROLL_RIGHT << 8) + SCROLL_TOP
            ld      bc, (SCROLL_RIGHT << 8) + SCROLL_TOP
            ld      a, (vars.scrollPixels)
            add     a, c
            ld      c, a
            ld      a, (vars.scrollRemain)
            ld      e, a
            ld      d, 10
            call    bitmapCopy
            ld      hl, (SCROLL_LEFT << 8) + SCROLL_TOP
            ld      a, (vars.scrollRemain)
            add     a, l
            ld      l, a
            ld      c, SCROLL_TOP
            ld      a, (vars.scrollPixels)
            ld      e, a
            call    bitmapCopy
            ld      l, SCROLL_TOP
            ld      b, SCROLL_LEFT
            ld      a, (vars.scrollPixels)
            add     SCROLL_TOP
            ld      c, a
            ld      a, (vars.scrollRemain)
            ld      e, a
            ld      a, (vars.scrollCursorKept)
            or      a
            jp z,   bitmapLeftCopy
            ; Первую исходную строку с инверсией не переносим.
            ld      l, SCROLL_TOP + 10
            ld      a, c
            add     10
            ld      c, a
            ld      a, e
            sub     10
            ld      e, a
bitmapLeftCopy:
            call    bitmapCopy

            ld      a, (vars.scrollRows)
            ld      b, a
            call    printFilesUpDownScroll
            scf
            ret

; Относительные HL/BC, D=ширина, E=высота. Нулевая высота — no-op.
; Сохраняет BC/DE/HL; AF/флаги разрушены. Цвет уже выбран.
bitmapCopy:
            ld      a, e
            or      a
            ret z
            push    hl
            push    bc
            push    de
            call    activePanelPos
            push    hl
            ld      h, b
            ld      l, c
            call    activePanelPos
            ld      b, h
            ld      c, l
            pop     hl
            call    bios_uiCopyRect
            pop     de
            pop     bc
            pop     hl
            ret
