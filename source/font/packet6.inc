; Построчный 6x8: полные байты экрана, два указателя на столбец.
; Внутренний вход нерекурсивный, аппаратные IRQ выключены.
    MODULE fast6
; HL=следующий байт текста. realMode: 0=Z с управлением, 1=RawZ,
; 2=RawLim; только в режиме 2 входной BC задаёт доступное число байтов.
; Все регистры разрушены. На выходе HL указывает на непрочитанный остаток:
; CF=1 — не нарисовано ничего, CF=0 — нарисована хотя бы одна четверка.
; Квота ограничивает чтение RawLim. Адрес экрана считается один раз,
; после каждой четвёрки DE продвигается на три столбца до правого края.
; При раннем выходе внешний RawLim вычитает реальный сдвиг HL, не квоту.
tryGroup:
    ld (text),hl
    ld hl,scan
    ld (scanRoute+1),hl
    ld a,20h
    ld (scanLow+1),a
    ld a,(realMode)
    or a
    jp z,checkEsc
    dec a
    jp nz,limitedMode
    ld a,1
    ld (scanLow+1),a
    jp stateReady
limitedMode:
    ld hl,scanRaw
    ld (scanRoute+1),hl
    jp stateReady
checkEsc:
    ld a,(bios_upper.escMode)
    inc a
    jp nz,emptyGroup
stateReady:
    ld hl,(bios_vars.cursorY)
    ld a,h
    and 3
    jp nz,emptyGroup
    ld a,l
    cp 7
    jp c,emptyGroup
    IF FONT_MODE
    ld hl,columnsPackedNormal
    ld de,columnsPackedInverse
    ELSE
    IFDEF FONT_ROTATE_ROWS
    ld hl,rotateRowsNormal
    ld de,rotateRowsInverse
    ELSE
    ld hl,columnsRawNormal
    ld de,columnsRaw
    ENDIF
    ENDIF
selectInverse:
    ; Обычное ядро не содержит NOP. Инверсное содержит CPL после слияния.
    ; Выбираем вход один раз перед пакетом, без проверки в пиксельном цикле.
    ld a,(bios_vars.inverse)
    or a
    jp z,routeReady
    ex de,hl
routeReady:
    ld (route+1),hl
    ld hl,(bios_vars.cursorY)
    ld a,h
    cp 181
    jp nc,emptyGroup
    ld a,h
    rrca
    rrca
    add a,90h
    ld d,a
    ld (startColumn+1),a
    ; Cap the first disallowed starting column by the right edge. A
    ; 6x8 quad uses three columns, so BEh is the first unusable start.
    ld a,(realMode)
    cp 2
    ld a,0BEh
    jp nz,endReady
    ld a,b
    or a
    ld a,0BEh
    jp nz,endReady
    ld a,c
    cp 64
    ld a,0BEh
    jp nc,endReady
    ld a,c
    rrca
    rrca
    and 15
    jp z,emptyGroup
    ld b,a
    add a,a
    add a,b
    add a,d
    cp 0BEh
    jp c,endReady
    ld a,0BEh
endReady:
    ld (endColumn+1),a
    ld a,l
    sub 7
    ld e,a
nextGroup:
    ld hl,(text)
    ; Выбор проверок один раз на пакет. Длина RawLim уже ограничена квотой.
scanRoute:
    jp scan
scanRaw:
    MACRO READ_CODE offset
        ld a,(hl)
        ld (codes+offset),a
        inc hl
    ENDM
    READ_CODE 0
    READ_CODE 1
    READ_CODE 2
    READ_CODE 3
    jp scanDone
scan:
    ; BC walks the four code bytes, leaving the VRAM pointer in DE.
    ld bc,codes
scanByte:
    ld a,(hl)
scanLow:
    cp 20h
    jp c,noGroup
    ld (bc),a
    inc bc
    inc hl
    ld a,c
    cp (codes+4) & 0FFh
    jp nz,scanByte
scanDone:
    ld (text),hl
route:
    IF FONT_MODE
    jp columnsPackedNormal
    ELSE
    IFDEF FONT_ROTATE_ROWS
    jp rotateRowsNormal
    ELSE
    jp columnsRawNormal
    ENDIF
    ENDIF
    IF !FONT_MODE
    IFDEF FONT_ROTATE_ROWS
    INCLUDE "rotate-rows.inc"
    ELSE
columnsRawNormal:
    ld hl,(codes+0)
    ld c,h
    ld h,FONT_ADDR >> 8
    ld b,h
    ld a,(bc)
    rrca
    rrca
    rrca
    rrca
    and 3
    ld (merge0_0Normal+1),a
    ld a,(hl)
    add a,a
    add a,a
merge0_0Normal: or 0
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    rrca
    rrca
    rrca
    rrca
    and 3
    ld (merge0_1Normal+1),a
    ld a,(hl)
    add a,a
    add a,a
merge0_1Normal: or 0
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    rrca
    rrca
    rrca
    rrca
    and 3
    ld (merge0_2Normal+1),a
    ld a,(hl)
    add a,a
    add a,a
merge0_2Normal: or 0
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    rrca
    rrca
    rrca
    rrca
    and 3
    ld (merge0_3Normal+1),a
    ld a,(hl)
    add a,a
    add a,a
merge0_3Normal: or 0
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    rrca
    rrca
    rrca
    rrca
    and 3
    ld (merge0_4Normal+1),a
    ld a,(hl)
    add a,a
    add a,a
merge0_4Normal: or 0
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    rrca
    rrca
    rrca
    rrca
    and 3
    ld (merge0_5Normal+1),a
    ld a,(hl)
    add a,a
    add a,a
merge0_5Normal: or 0
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    rrca
    rrca
    rrca
    rrca
    and 3
    ld (merge0_6Normal+1),a
    ld a,(hl)
    add a,a
    add a,a
merge0_6Normal: or 0
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    rrca
    rrca
    rrca
    rrca
    and 3
    ld (merge0_7Normal+1),a
    ld a,(hl)
    add a,a
    add a,a
merge0_7Normal: or 0
    ld (de),a
    ld a,e
    sub 7
    ld e,a
    inc d
    ld hl,(codes+1)
    ld c,h
    ld h,FONT_ADDR >> 8
    ld b,h
    ld a,(bc)
    rrca
    rrca
    and 0Fh
    ld (merge1_0Normal+1),a
    ld a,(hl)
    rrca
    rrca
    rrca
    rrca
    and 0F0h
merge1_0Normal: or 0
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    rrca
    rrca
    and 0Fh
    ld (merge1_1Normal+1),a
    ld a,(hl)
    rrca
    rrca
    rrca
    rrca
    and 0F0h
merge1_1Normal: or 0
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    rrca
    rrca
    and 0Fh
    ld (merge1_2Normal+1),a
    ld a,(hl)
    rrca
    rrca
    rrca
    rrca
    and 0F0h
merge1_2Normal: or 0
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    rrca
    rrca
    and 0Fh
    ld (merge1_3Normal+1),a
    ld a,(hl)
    rrca
    rrca
    rrca
    rrca
    and 0F0h
merge1_3Normal: or 0
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    rrca
    rrca
    and 0Fh
    ld (merge1_4Normal+1),a
    ld a,(hl)
    rrca
    rrca
    rrca
    rrca
    and 0F0h
merge1_4Normal: or 0
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    rrca
    rrca
    and 0Fh
    ld (merge1_5Normal+1),a
    ld a,(hl)
    rrca
    rrca
    rrca
    rrca
    and 0F0h
merge1_5Normal: or 0
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    rrca
    rrca
    and 0Fh
    ld (merge1_6Normal+1),a
    ld a,(hl)
    rrca
    rrca
    rrca
    rrca
    and 0F0h
merge1_6Normal: or 0
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    rrca
    rrca
    and 0Fh
    ld (merge1_7Normal+1),a
    ld a,(hl)
    rrca
    rrca
    rrca
    rrca
    and 0F0h
merge1_7Normal: or 0
    ld (de),a
    ld a,e
    sub 7
    ld e,a
    inc d
    ld hl,(codes+2)
    ld c,h
    ld h,FONT_ADDR >> 8
    ld b,h
    ld a,(bc)
    ld (merge2_0Normal+1),a
    ld a,(hl)
    rrca
    rrca
    and 0C0h
merge2_0Normal: or 0
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    ld (merge2_1Normal+1),a
    ld a,(hl)
    rrca
    rrca
    and 0C0h
merge2_1Normal: or 0
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    ld (merge2_2Normal+1),a
    ld a,(hl)
    rrca
    rrca
    and 0C0h
merge2_2Normal: or 0
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    ld (merge2_3Normal+1),a
    ld a,(hl)
    rrca
    rrca
    and 0C0h
merge2_3Normal: or 0
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    ld (merge2_4Normal+1),a
    ld a,(hl)
    rrca
    rrca
    and 0C0h
merge2_4Normal: or 0
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    ld (merge2_5Normal+1),a
    ld a,(hl)
    rrca
    rrca
    and 0C0h
merge2_5Normal: or 0
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    ld (merge2_6Normal+1),a
    ld a,(hl)
    rrca
    rrca
    and 0C0h
merge2_6Normal: or 0
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    ld (merge2_7Normal+1),a
    ld a,(hl)
    rrca
    rrca
    and 0C0h
merge2_7Normal: or 0
    ld (de),a
    jp done
columnsRaw:
    ld hl,(codes+0)
    ld c,h
    ld h,FONT_ADDR >> 8
    ld b,h
    ld a,(bc)
    rrca
    rrca
    rrca
    rrca
    and 3
    ld (merge0_0+1),a
    ld a,(hl)
    add a,a
    add a,a
merge0_0: or 0
invRaw0_0: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    rrca
    rrca
    rrca
    rrca
    and 3
    ld (merge0_1+1),a
    ld a,(hl)
    add a,a
    add a,a
merge0_1: or 0
invRaw0_1: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    rrca
    rrca
    rrca
    rrca
    and 3
    ld (merge0_2+1),a
    ld a,(hl)
    add a,a
    add a,a
merge0_2: or 0
invRaw0_2: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    rrca
    rrca
    rrca
    rrca
    and 3
    ld (merge0_3+1),a
    ld a,(hl)
    add a,a
    add a,a
merge0_3: or 0
invRaw0_3: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    rrca
    rrca
    rrca
    rrca
    and 3
    ld (merge0_4+1),a
    ld a,(hl)
    add a,a
    add a,a
merge0_4: or 0
invRaw0_4: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    rrca
    rrca
    rrca
    rrca
    and 3
    ld (merge0_5+1),a
    ld a,(hl)
    add a,a
    add a,a
merge0_5: or 0
invRaw0_5: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    rrca
    rrca
    rrca
    rrca
    and 3
    ld (merge0_6+1),a
    ld a,(hl)
    add a,a
    add a,a
merge0_6: or 0
invRaw0_6: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    rrca
    rrca
    rrca
    rrca
    and 3
    ld (merge0_7+1),a
    ld a,(hl)
    add a,a
    add a,a
merge0_7: or 0
invRaw0_7: cpl
    ld (de),a
    ld a,e
    sub 7
    ld e,a
    inc d
    ld hl,(codes+1)
    ld c,h
    ld h,FONT_ADDR >> 8
    ld b,h
    ld a,(bc)
    rrca
    rrca
    and 0Fh
    ld (merge1_0+1),a
    ld a,(hl)
    rrca
    rrca
    rrca
    rrca
    and 0F0h
merge1_0: or 0
invRaw1_0: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    rrca
    rrca
    and 0Fh
    ld (merge1_1+1),a
    ld a,(hl)
    rrca
    rrca
    rrca
    rrca
    and 0F0h
merge1_1: or 0
invRaw1_1: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    rrca
    rrca
    and 0Fh
    ld (merge1_2+1),a
    ld a,(hl)
    rrca
    rrca
    rrca
    rrca
    and 0F0h
merge1_2: or 0
invRaw1_2: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    rrca
    rrca
    and 0Fh
    ld (merge1_3+1),a
    ld a,(hl)
    rrca
    rrca
    rrca
    rrca
    and 0F0h
merge1_3: or 0
invRaw1_3: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    rrca
    rrca
    and 0Fh
    ld (merge1_4+1),a
    ld a,(hl)
    rrca
    rrca
    rrca
    rrca
    and 0F0h
merge1_4: or 0
invRaw1_4: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    rrca
    rrca
    and 0Fh
    ld (merge1_5+1),a
    ld a,(hl)
    rrca
    rrca
    rrca
    rrca
    and 0F0h
merge1_5: or 0
invRaw1_5: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    rrca
    rrca
    and 0Fh
    ld (merge1_6+1),a
    ld a,(hl)
    rrca
    rrca
    rrca
    rrca
    and 0F0h
merge1_6: or 0
invRaw1_6: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    rrca
    rrca
    and 0Fh
    ld (merge1_7+1),a
    ld a,(hl)
    rrca
    rrca
    rrca
    rrca
    and 0F0h
merge1_7: or 0
invRaw1_7: cpl
    ld (de),a
    ld a,e
    sub 7
    ld e,a
    inc d
    ld hl,(codes+2)
    ld c,h
    ld h,FONT_ADDR >> 8
    ld b,h
    ld a,(bc)
    ld (merge2_0+1),a
    ld a,(hl)
    rrca
    rrca
    and 0C0h
merge2_0: or 0
invRaw2_0: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    ld (merge2_1+1),a
    ld a,(hl)
    rrca
    rrca
    and 0C0h
merge2_1: or 0
invRaw2_1: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    ld (merge2_2+1),a
    ld a,(hl)
    rrca
    rrca
    and 0C0h
merge2_2: or 0
invRaw2_2: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    ld (merge2_3+1),a
    ld a,(hl)
    rrca
    rrca
    and 0C0h
merge2_3: or 0
invRaw2_3: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    ld (merge2_4+1),a
    ld a,(hl)
    rrca
    rrca
    and 0C0h
merge2_4: or 0
invRaw2_4: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    ld (merge2_5+1),a
    ld a,(hl)
    rrca
    rrca
    and 0C0h
merge2_5: or 0
invRaw2_5: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    ld (merge2_6+1),a
    ld a,(hl)
    rrca
    rrca
    and 0C0h
merge2_6: or 0
invRaw2_6: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    ld (merge2_7+1),a
    ld a,(hl)
    rrca
    rrca
    and 0C0h
merge2_7: or 0
invRaw2_7: cpl
    ld (de),a
    jp done
    ENDIF
    ELSE
    INCLUDE "packed-columns.inc"
    ENDIF
done:
    ; The pixel kernel leaves DE at the bottom of the third column.
    ld a,e
    sub 7
    ld e,a
    inc d
    ld a,d
endColumn: cp 0BEh
    jp c,nextGroup
noGroup:
    ; A rejected control/NUL quad leaves D and the text pointer intact.
    ; Recover both progress and cursor X from D; no separate quad count.
    ld a,d
startColumn: cp 0
    jp z,emptyGroup
    sub 90h
    add a,a
    add a,a
    ld (bios_vars.cursorX),a
    cp 192
    jp c,groupReturn
    ld hl,(bios_vars.cursorY)
    call fontLineEnd
groupReturn:
    ld hl,(text)
    or a ; CF=0, напечатана хотя бы одна четвёрка.
    ret
emptyGroup:
    ld hl,(text)
    scf
    ret
realMode: DB 0
text: DW 0
codes: DS 4
    ENDMODULE
