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
            ; ASCII-глиф берётся через таблицу диапазонов внешнего MXFN.
            ld c,a
            and 0E0h
            rrca
            rrca
            rrca
            rrca
            ld e,a
            ld d,0
            ld hl,FONT_RANGES_FIELD
            add hl,de
            ld e,(hl)
            inc hl
            ld d,(hl)
            ld a,c
            and 01Fh
            ld l,a
            ld h,0
            add hl,hl
            add hl,hl
            add hl,hl
            add hl,de
            ld de,FONT_ADDR
            add hl,de
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
            inc hl
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
