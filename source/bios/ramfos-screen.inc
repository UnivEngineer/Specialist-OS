;----------------------------------------------------------------------------
; MXOS
; Прочитать символ под курсором RAMFOS, включая прямо нарисованный игрой.
; вход: MAIN, установлен FNT1, C63Eh = координаты нижнего левого пикселя.
; выход: A = первый совпавший печатный код или FFh. BC/DE/HL сохранены,
; флаги изменены. Видеопамять, цвет, курсор и состояние шрифта сохранены.
;----------------------------------------------------------------------------
ramfosGetGlyph:
    push bc
    push de
    push hl
    ld hl,(ramfos_cursorPos)
    ld a,(FONT_ENGINE_HEADER+5)
    ld b,a
    ld a,h
    add a,a
    and 7
    ld c,a                    ; Смещение от старшего бита видеобайта
    ld a,h
    rrca
    rrca
    and 3Fh
    add a,high MX_SCREEN_BASE
    ld h,a
    ld a,l
    sub 7
    ld l,a
    ld de,ramfosScreenRows
.row:
    push bc
    push hl
    ld a,(hl)
    inc h
    ld l,(hl)
    ld h,a
    ld a,c
    or a
    jp z,.bits
.shift:
    add hl,hl
    dec c
    jp nz,.shift
.bits:
    xor a
.bit:
    add hl,hl
    rla
    dec b
    jp nz,.bit
    ld (de),a
    inc de
    pop hl
    pop bc
    inc l
    ld a,e
    cp low (ramfosScreenRows+8)
    jp nz,.row
    ; Маска отбросит служебные копии битов в упакованных строках шрифта.
    ld a,0FFh
.widthMask:
    add a,a
    dec b
    jp nz,.widthMask
    cpl
    ld (ramfosGlyphMask),a
    ld a,(mon2_vars.inverse)
    or a
    jp z,.inverseReady
    ld a,0FFh
.inverseReady:
    ld (ramfosGlyphInverse),a
    ld c,20h
.candidate:
    ld hl,ramfosScreenRows
    ld b,0
.compare:
    push hl
    call ramfosGlyphRow
    ld d,a
    ld a,(ramfosGlyphInverse)
    xor d
    ld d,a
    ld a,(ramfosGlyphMask)
    and d
    pop hl
    cp (hl)
    inc hl
    jp nz,.miss
    inc b
    ld a,b
    cp 8
    jp nz,.compare
    ld a,c
    jp .done
.miss:
    inc c
    jp nz,.candidate
    ld a,0FFh
.done:
    pop hl
    pop de
    pop bc
    ret

; B = строка 0..7, C = код. Вернуть в A биты текущего глифа FNT1.
; Формат и физическая таблица берутся из заголовка присоединённого движка.
ramfosGlyphRow:
    ld hl,(FONT_ENGINE_HEADER+10)
    ld a,(FONT_ENGINE_HEADER+7)
    cp 3
    jp z,.compact6
    cp FONT_FORMAT_4_COMPACT
    jp z,.compact4
    cp FONT_FORMAT_KOI7
    jp nc,.koi7
    ld l,c
    ld a,h
    add a,b
    ld h,a
    ld a,(FONT_ENGINE_HEADER+7)
    cp FONT_FORMAT_4_FAST
    ld a,(hl)
    ret nz
.highNibble:
    rrca
    rrca
    rrca
    rrca
    ret
.compact6:
    dec h                     ; Карта 256 кодов непосредственно перед глифами
    ld l,c
    ld a,(hl)
    inc h
    ld l,0
    jp .linear
.compact4:
    ld a,c
    or a
    rra
.linear:
    ld d,0
    ld e,a
    ex de,hl
    add hl,hl
    add hl,hl
    add hl,hl
    add hl,de
    ld d,0
    ld e,b
    add hl,de
    ld a,(FONT_ENGINE_HEADER+7)
    cp FONT_FORMAT_4_COMPACT
    ld a,(hl)
    ret nz
    ld d,a
    ld a,c
    and 1
    ld a,d
    ret nz
    jp .highNibble
.koi7:
    ld a,c
    and 7Fh
    ld l,a
    ld a,b
    and 3
    add a,h
    ld h,a
    ld a,b
    and 4
    jp z,.read
    ld a,l
    or 80h
    ld l,a
.read:
    ld a,(hl)
    ret
;----------------------------------------------------------------------------
; MXOS
; Установить строку состояния по C82Dh.
; вход: MAIN, HL = адрес идентификатора и следующей за ним строки ASCIIZ.
; выход: при совпадении идентификатора состояние сохранено, Z=1.
; После вывода ID записан, курсор обновлён, HL как после ramfosString.
;----------------------------------------------------------------------------
ramfosBottomLine:
    ld a,(ramfosLineId)
    cp (hl)
    ret z
    ld a,(hl)
    inc hl
    push af
    call ramfosString
    pop af
    ld (ramfosLineId),a
    ret
