            include "../../source/include/mxos.inc"
            include "api.inc"
; MODE: 0 — BIOS, 1 — готовые сдвиги, 2 — 8x8, 3 — BIOS с новым drawChar.
; BATCH: 0 — отдельный символ C, 1 — C печатных байт общего буфера.
; Оба входа предназначены для выбранного варианта сборки. AF/BC/DE/HL
; сохраняются. Позиция задаётся общей переменной BIOS, строки меняет стенд.
            org 0EB00h
scalarEntry:
            IF (MODE == 0 || MODE == 3) && BATCH == 0
            jp bios_printChar
batchEntry: jp bios_printChar
            ELSE
            jp render
batchEntry: jp render
            ENDIF
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
            IF BATCH
            ld a, c
            or a
            jp z, restore
            cp BUFFER_SIZE+1
            jp nc, restore
            add a, 0CCh
            ld (endBuffer+1), a
            ld a, 0CCh
            ld (readChar+1), a
readChar:   ld a, (BUFFER_ADDR)
            ld c, a
            ENDIF
glyphBody:
            IF MODE == 0 || MODE == 3
            call bios_printChar
            ELSE
            ld hl, (bios_vars.cursorY)
            ld b, h
            inc h
            inc h
            inc h
            IF MODE == 2
            inc h
            ENDIF
            ld (bios_vars.cursorY), hl
            ld h, b
            IF MODE == 1
            ld a, h
            and 3
            ld b, a
            ENDIF
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
            IF MODE == 2
            ld a, h
            add a, FONT8_ADDR/256
            ld h, a
eightBytes:
            DUP 7
            ld a, (hl)
            ld (de), a
            inc l
            inc e
            EDUP
            ld a, (hl)
            ld (de), a
            jp glyphDone
            ELSE
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
            add a, 38h
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
            ENDIF
            ENDIF
glyphDone:
            IF BATCH
; На символ нет CALL рендера и сохранения общего набора регистров.
; Указатель лежит в операнде LDA; буфер целиком внутри страницы FFxxh.
            ld hl, readChar+1
            inc (hl)
            ld a, (hl)
endBuffer:  cp 0E0h
            jp c, readChar
            ENDIF
restore:    pop hl
            pop de
            pop bc
            pop af
            ret
            assert $ <= 0F400h
