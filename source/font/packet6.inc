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
    xor a
    ld (produced),a
    ld hl,nextGroup
    ld (groupNext+1),hl
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
    ; RawLim: BC — общий остаток байтов. Квота = min(BC / 4, 64).
    ; >=256 байт дают 64 четверки, иначе два RRC и AND дают floor(C/4).
    ; Считаем четверки, поэтому 256 байт не требуют условного «0 = 256».
    ld hl,limitedNext
    ld (groupNext+1),hl
    ld hl,scanRaw
    ld (scanRoute+1),hl
    ld a,b
    or a
    ld a,64
    jp nz,limitedQuota
    ld a,c
    rrca
    rrca
    and 63
limitedQuota:
    ; LD не меняет ZF: от OR B либо AND 63 здесь осталось «квота = 0».
    ld (limitedNext+1),a
    jp z,noGroup
    jp stateReady
checkEsc:
    ld a,(bios_upper.escMode)
    inc a
    jp nz,noGroup
stateReady:
    ld hl,(bios_vars.cursorY)
    ld a,h
    and 3
    jp nz,noGroup
    ld a,l
    cp 7
    jp c,noGroup
    IF FONT_MODE
    ld hl,columnsPackedNormal
    ld de,columnsPackedInverse
    ELSE
    ld hl,columnsRawNormal
    ld de,columnsRaw
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
    jp nc,noGroup
    ld a,h
    rrca
    rrca
    add a,90h
    ld d,a
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
scanStop:
    pop de
    jp noGroup
scan:
    push de
    ld de,codes
    ld b,4
scanByte:
    ld a,(hl)
scanLow:
    cp 20h
    jp c,scanStop
scanAccepted:
    ld (de),a
    inc de
    inc hl
    dec b
    jp nz,scanByte
    pop de
scanDone:
    ld (text),hl
route:
    IF FONT_MODE
    jp columnsPackedNormal
    ELSE
    jp columnsRawNormal
    ENDIF
limitedNext:
    ; 8-битная квота живёт в операнде LD A,n: регистры нужны пикселям.
    ; Только после успешно нарисованной четверки уменьшаем её на один.
    ; Здесь нет загрузки/записи 16-битного остатка и повторного hasFour.
    ; Все ядра fast6 нерекурсивны, IRQ выключены, самомодификация допустима.
    ld a,0
    dec a
    ld (limitedNext+1),a
    jp nz,nextGroup
noGroup:
    ; Курсор фиксируется один раз по реально нарисованным четвёркам.
    ; При остановке Z внутри четвёрки text всё ещё указывает на её начало.
    ld a,(produced)
    or a
    jp z,emptyGroup
    add a,a
    add a,a
    ld b,a
    add a,a
    add a,b
    ld hl,bios_vars.cursorX
    add a,(hl)
    ld (hl),a
    cp 192
    jp c,groupReturn
    ld hl,(bios_vars.cursorY)
    call printChar_c0A
groupReturn:
    ld hl,(text)
    or a ; CF=0, напечатана хотя бы одна четвёрка.
    ret
emptyGroup:
    ld hl,(text)
    scf
    ret
    IF !FONT_MODE
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
    ELSE
    INCLUDE "packed-columns.inc"
    ENDIF
done:
    ; Продолжаем экранный указатель, как в PAGED. Последняя строка ядра
    ; оставила E на +7; следующий полный столбец получается через INC D.
    ld a,e
    sub 7
    ld e,a
    inc d
    ld hl,produced
    inc (hl)
    ld a,d
    cp 0BEh ; Для следующей четвёрки нужны ещё три полных столбца.
    jp nc,noGroup
groupNext:
    jp nextGroup
realMode: DB 0
produced: DB 0
text: DW 0
codes: DS 4
    ENDMODULE
