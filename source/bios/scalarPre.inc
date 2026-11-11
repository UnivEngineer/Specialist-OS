; Скалярный COLPRE 6x8: тот же шрифт и пять плоскостей 10 КиБ, что у четвёрок.
; C=код; printChar_alt уже проверил FONT_ADDR, preBase и эпоху.
; Регистры разрушены. Обычный курсор обновляется локально, редкий перенос
; перед рисованием использует общий printChar_position.
;
; HL=один столбец битмапа, DE=экран, B=маска сохранения фона.
; Рисуем столбец полностью: второй указатель на глиф и временные SMC-слияния
; на каждой строке не нужны. Для пересечения границы сохраняем исходные
; указатели на стеке один раз, затем INC D переходит к соседнему столбцу.
; Раскладка страниц строк даёт INC H вместо расчёта следующего адреса.
;
; Ядра обычного и инверсного вывода раздельные: в обычном нет XOR/NOP.
; Переход выбирается один раз на глиф. Инверсия меняет только шесть пикселей
; знакоместа, фон вне маски остаётся неизменным.
    MODULE scalarPre

    MACRO COLPRE_TWO columnKernel, offset, keepLeft, keepRight
    ld a,(fast6.preBase+1)
    add a,offset
    ld h,a
    ld b,keepLeft
    push hl
    push de
    call columnKernel
    pop de
    pop hl
    inc d
    ld a,h
    add a,8
    ld h,a
    ld b,keepRight
    jp columnKernel
    ENDM

    MACRO COLPRE_BODY columnKernel
draw:
    ; Обычный курсорный путь встроен, как в standalone COLPRE. Код остаётся
    ; в C, фаза — в B: нет CALL/RET и SMC-записей кода/фазы на каждый глиф.
    ld hl,(bios_vars.cursorY)
    ld a,h
    cp 190
    jp nc,wrap
    and 3
    ld b,a
    ld a,h
    rrca
    rrca
    and 3Fh
    add a,90h
    ld d,a
    ld a,l
    sub 7
    ld e,a
    inc h
    inc h
    inc h
    ld (bios_vars.cursorY),hl
    ld l,c
    ld a,b
phaseReady:
    or a
    jp z,phase0
    dec a
    ld h,FONT_ADDR >> 8
    ld b,0C0h
    jp z,columnKernel
    dec a
    jp nz,phase6
    ; X mod 8 = 4: левый столбец b>>2, правый b<<6.
    COLPRE_TWO columnKernel, 8, 0F0h, 03Fh
phase6:
    ; X mod 8 = 6: левый столбец b>>4, правый b<<4.
    COLPRE_TWO columnKernel, 24, 0FCh, 00Fh
phase0:
    ; X mod 8 = 0: готовый b<<2 занимает один столбец.
    ; Для X mod 8 = 2 выше берётся b непосредственно из шрифта.
    ld a,(fast6.preBase+1)
    ld h,a
    ld b,3
    jp columnKernel
wrap:
    ; X=190/191 и некорректные старшие значения требуют штатного переноса
    ; до рисования. Редкий путь сохраняет код на стеке, общий helper меняет C.
    ld a,c
    push af
    call printChar_position
    ld d,b
    ld e,c
    ld a,h
    inc a
    and 3
    ld h,a
    pop af
    ld l,a
    ld a,h
    jp phaseReady
    ENDM

draw:
    ld a,(bios_vars.inverse)
    or a
    jp nz,inverse.draw
    ; Обычный путь входит в setup без дополнительного CALL/JMP.
    MODULE normal
    COLPRE_BODY scalarPre.columnNormal
    ENDMODULE

    MODULE inverse
    COLPRE_BODY scalarPre.columnInverse
    ENDMODULE

columnNormal:
    DUP 7
        ld a,(de)
        and b
        or (hl)
        ld (de),a
        inc h
        inc de
    EDUP
    ld a,(de)
    and b
    or (hl)
    ld (de),a
    ret
columnInverse:
    ; XOR только в области глифа. C свободен, маска второго столбца
    ; задаётся вызывающим непосредственно после возврата из первого столбца.
    ld a,b
    cpl
    ld c,a
    DUP 7
        ld a,(de)
        and b
        or (hl)
        xor c
        ld (de),a
        inc h
        inc de
    EDUP
    ld a,(de)
    and b
    or (hl)
    xor c
    ld (de),a
    ret
    ENDMODULE
