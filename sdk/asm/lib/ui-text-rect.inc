; Shared monochrome/one-color copy of 8 glyph rows in each 10px text cell.
; HL=источник, BC=назначение, D=ширина в байтах, E=высота в пикселях.
; D>0; E=10..250, кратно 10; обе области внутри VRAM, без перехода Y через 256.
; Горизонтальное перекрытие допустимо в обе стороны.
; Вертикальное перекрытие допустимо в обе стороны. Строки 0/9 каждой ячейки
; должны уже иметь одинаковый пустой фон. Инверсия только глифов допустима;
; выделение, меняющее строки 0/9, снимает/исключает вызывающий.
; Цвет задаёт вызывающий. AF/BC/DE/HL разрушены; SP восстановлен.
; Нереентерабелен; IRQ запрещены контрактом MXOS. Пока SP указывает на VRAM,
; нет CALL/PUSH/POP кроме собственно переноса; код и рабочие данные вне VRAM.
uiCopyTextRect:
    MODULE uiTextRect
    ld a,e
    or a
    ret z
    ld (height),a
    ld (source),hl
    ld h,b
    ld l,c
    ld (destination),hl
    ld a,d
    ld (width),a
    ld a,e
    ld d,0
countRows:
    inc d
    sub 10
    jp nz,countRows
    ld a,d
    ld (rows),a
    ld hl,0
    add hl,sp
    ld (restoreSP+1),hl
    ; Select the column order separately from the order of rows.
    ld a,1
    ld (columnStep),a
    ld hl,(source)
    ld a,h
    cp b
    jp nc,columnsReady
    ld a,255
    ld (columnStep),a
    ld a,(width)
    dec a
    add a,h
    ld h,a
    ld (source),hl
    ld hl,(destination)
    ld a,(width)
    dec a
    add a,h
    ld h,a
    ld (destination),hl
columnsReady:
    ld hl,(source)
    ; Y alone determines the row order, independently of X.
    ld a,c
    cp l
    jp c,setForward
    jp setBackward
setBackward:
    ld a,1
    ld (direction),a
    jp backward
setForward:
    xor a
    ld (direction),a
    jp forward
backward:
    ld hl,(destination)
    ld a,(height)
    add a,l
    ld l,a
    dec l
    ld sp,hl
    ld hl,(source)
    ld a,(height)
    add a,l
    ld l,a
    dec l
    ld a,(rows)
    ld b,a
backRow:
    DUP 4
    dec l
    ld d,(hl)
    dec l
    ld e,(hl)
    push de
    EDUP
    dec l
    dec l
    dec sp
    dec sp
    dec b
    jp nz,backRow
    jp nextColumn
forward:
    ld hl,(source)
    inc l
    ld sp,hl
    ld hl,(destination)
    inc l
    ld a,(rows)
    ld b,a
forwardRow:
    DUP 4
    pop de
    ld (hl),e
    inc l
    ld (hl),d
    inc l
    EDUP
    inc l
    inc l
    inc sp
    inc sp
    dec b
    jp nz,forwardRow
nextColumn:
    ld a,(columnStep)
    ld hl,(source)
    add a,h
    ld h,a
    ld (source),hl
    ld a,(columnStep)
    ld hl,(destination)
    add a,h
    ld h,a
    ld (destination),hl
    ld a,(width)
    dec a
    ld (width),a
    jp z,restoreSP
    ld a,(direction)
    or a
    jp z,forward
    jp backward
restoreSP:
    ld sp,0
    ret
source:         DW 0
destination:    DW 0
height:         DB 0
rows:           DB 0
width:          DB 0
direction:      DB 0
columnStep:     DB 1
    ENDMODULE
