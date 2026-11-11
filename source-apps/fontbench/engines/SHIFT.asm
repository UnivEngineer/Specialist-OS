    IFNDEF FONT_ENGINE_DLL
            include "../../../source/include/mxos.inc"
    ENDIF
; Посимвольный пререндер 12 КБ автономного FONTBEN.
; C — код, координаты и курсор общие с BIOS. Регистры сохранены.
            org SHIFT_ORIGIN
scalarEntry:
            jp render
batchEntry: jp render
newLine:    push af
            push hl
            ld hl, (bios_vars.cursorY)
            ld h, 0
            ld a, l
            add a, 8
            ld l, a
            ld (bios_vars.cursorY), hl
            pop hl
            pop af
            ret
render:     push af
            push bc
            push de
            push hl
glyphBody:
            ld hl, (bios_vars.cursorY)
            ld b, h
            inc h
            inc h
            inc h
            ld (bios_vars.cursorY), hl
            ld h, b
            ld a, h
            and 3
            ld b, a
            ld a, h
            rrca
            rrca
            and 3Fh
            add a, 90h
            ld h, a
            ld a, l
            sub 7
            ld l, a
            ex de, hl
            ld h, 0
            ld l, c
            add hl, hl
            add hl, hl
            add hl, hl
            ld a, b
            or a
            jp z, phase0
            dec a
            jp z, phase2
            add hl, hl
            dec a
            jp z, phase4
phase6:     ld a, h
            add a, 50h
            ld h, a
            ld bc, 0FC0Fh
            jp twoBytes
phase4:     ld a, h
            add a, 40h
            ld h, a
            ld bc, 0F03Fh
            jp twoBytes
phase2:     ld a, h
            add a, 28h
            ld h, a
            ld c, 0C0h
            jp oneByte
phase0:     ld a, h
            add a, 30h
            ld h, a
            ld c, 3
oneByte:
            DUP 7
            ld a, (de)
            and c
            or (hl)
            ld (de), a
            inc l
            inc e
            EDUP
            ld a, (de)
            and c
            or (hl)
            ld (de), a
            jp glyphDone
twoBytes:
            DUP 7
            ld a, (de)
            and b
            or (hl)
            ld (de), a
            inc l
            inc d
            ld a, (de)
            and c
            or (hl)
            ld (de), a
            inc l
            dec d
            inc e
            EDUP
            ld a, (de)
            and b
            or (hl)
            ld (de), a
            inc l
            inc d
            ld a, (de)
            and c
            or (hl)
            ld (de), a
glyphDone:
rendererReturn:
restore:    pop hl
            pop de
            pop bc
            pop af
            ret
            assert $ <= 0F400h
