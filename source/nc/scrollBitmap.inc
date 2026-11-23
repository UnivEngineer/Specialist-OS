; Битмап-прокрутка 1/2/3 колонок NC через локальный стековый копировщик.
; Вход B=1 вперёд, B=2 назад; vars.scrollRows=модуль шага.
; При scrollCursorKept инверсные пиксели исключаются из копии. Цветовое
; выделение переносится с обычным цветом из таблицы ячеек.
; Атрибуты ячеек хранятся в vars.rowColors; соседние цвета объединяются.
; CF=1: список и новый курсор нарисованы; AF/BC/DE/HL разрушены.
; CF=0: шаг больше колонки, нужен текстовый путь; BC/DE/HL сохранены.
; Межколоночный перенос делается до изменения его источника.
; В каждой ячейке высотой 10 копируются только 8 строк глифа:
; пустые строки 0 и 9 остаются на месте. Выделение снимается или исключается.
; Каталог берётся из кэша. Большие шаги идут текстовым путём.

scrollBitmapPanel:
            ld      a, (vars.scrollRows)
            cp      P_FILE_LIST_H + 1
            ret nc
            push    bc
            call    lf_UpdateViewport
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
            ld a,(ncFontWidth)
            cp 6
            jp nz,bitmapAdaptive
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
    IF !ENABLE_COLOR
            ld      a, (vars.scrollCursorKept)
            or      a
            jp z,   bitmapRightCopy
            ; Последняя исходная строка содержит старую инверсию.
            ld      a, e
            sub     10
            ld      e, a
bitmapRightCopy:
    ENDIF
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
    IF !ENABLE_COLOR
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
    ENDIF
            call    bitmapCopy

            ld      a, (vars.scrollRows)
            ld      b, a
            call    printFilesUpDownScroll
            scf
            ret

; Runtime geometry for 1 or 3 columns. The old selection is already erased;
; move each column before consuming the boundary strip from its neighbour.
; Forward visits columns left-to-right, backward right-to-left.
bitmapAdaptive:
            ld a,b
            ld (bitmapDirection),a
            ld a,(ncColumns)
            ld (bitmapColumnsLeft),a
            ld a,SCROLL_LEFT
            ld (bitmapColumn),a
            dec b
            jp z,bitmapAdaptiveForward
            ld a,(ncColumns)
            dec a
            jp z,bitmapAdaptiveBackward
            ld b,a
            ld a,(ncColumnStep)
            rrca
            rrca
            ld c,a
            ld a,SCROLL_LEFT
bitmapLastColumn:
            add a,c
            dec b
            jp nz,bitmapLastColumn
            ld (bitmapColumn),a
            jp bitmapAdaptiveBackward
bitmapAdaptiveForward:
            ld a,(bitmapColumn)
            ld h,a
            ld b,a
            ld c,SCROLL_TOP
            ld a,(vars.scrollPixels)
            add a,SCROLL_TOP
            ld l,a
            ld a,(vars.scrollRemain)
            ld e,a
            ld a,(ncCellBytes)
            ld d,a
            call bitmapCopy
            call bitmapColumnDone
            jp z,bitmapAdaptivePrint
            call bitmapNextColumn
            ld a,(vars.scrollRemain)
            add a,SCROLL_TOP
            ld c,a
            ld l,SCROLL_TOP
            ld a,(vars.scrollPixels)
            ld e,a
            call bitmapCopy
            ld a,h
            ld (bitmapColumn),a
            jp bitmapAdaptiveForward
bitmapAdaptiveBackward:
            ld a,(bitmapColumn)
            ld h,a
            ld b,a
            ld l,SCROLL_TOP
            ld a,(vars.scrollPixels)
            add a,SCROLL_TOP
            ld c,a
            ld a,(vars.scrollRemain)
            ld e,a
            ld a,(ncCellBytes)
            ld d,a
            call bitmapCopy
            call bitmapColumnDone
            jp z,bitmapAdaptivePrint
            ld a,(ncColumnStep)
            rrca
            rrca
            ld c,a
            ld a,h
            sub c
            ld h,a
            ld a,(vars.scrollRemain)
            add a,SCROLL_TOP
            ld l,a
            ld c,SCROLL_TOP
            ld a,(vars.scrollPixels)
            ld e,a
            call bitmapCopy
            ld a,h
            ld (bitmapColumn),a
            jp bitmapAdaptiveBackward
bitmapColumnDone:
            ld a,(bitmapColumnsLeft)
            dec a
            ld (bitmapColumnsLeft),a
            ret
bitmapNextColumn:
            ld a,(ncColumnStep)
            rrca
            rrca
            add a,h
            ld h,a
            ret
bitmapAdaptivePrint:
            ld a,(bitmapDirection)
            dec a
            ld a,0
            jp nz,bitmapAdaptiveFirstSlot
            ld a,(vars.scrollRows)
            ld c,a
            ld a,(ncVisibleCount)
            sub c
bitmapAdaptiveFirstSlot:
            ld (bitmapSlot),a
            ld a,(vars.scrollRows)
            ld b,a
bitmapAdaptiveNewRow:
            push bc
            ld a,(bitmapSlot)
            call ncSlotPosition
            ld a,(bitmapSlot)
            ld c,a
            ld b,0
            ld hl,(vars.fileListPtr)
            add hl,bc
            call printPanelFile
            ld hl,bitmapSlot
            inc (hl)
            pop bc
            dec b
            jp nz,bitmapAdaptiveNewRow
            COLOR COLOR_CURSOR
            call drawCursor
            COLOR COLOR_FILE
            scf
            ret
bitmapDirection:    DB 0
bitmapColumnsLeft:  DB 0
bitmapColumn:       DB 0
bitmapSlot:         DB 0

; Относительные HL/BC, D=ширина, E=высота, кратная 10. Ноль — no-op.
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
    IF NC_FILE_COLORS
            call    ncCopyColoredText
    ELSE
            call    uiCopyTextRect
    ENDIF
            pop     de
            pop     bc
            pop     hl
            ret
