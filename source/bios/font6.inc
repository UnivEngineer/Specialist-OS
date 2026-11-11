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
    ld hl,(bios_upper.charGen)
    ld a,h
    cp FONT_ADDR >> 8
    jp nz,noGroup
    ld a,l
    or a
    jp nz,noGroup
    ld hl,(bios_vars.cursorY)
    ld a,h
    and 3
    jp nz,noGroup
    ld a,l
    cp 7
    jp c,noGroup
    call preValid
    ld hl,columnsRawNormal
    ld de,columnsRaw
    jp z,selectInverse
    ; Старшие байты пяти построчных фаз меняются один раз на пакет.
    ld a,(preBase+1)
    ld (pre0+1),a
    ld (pre0Normal+1),a
    add a,24
    ld (pre1+1),a
    ld (pre1Normal+1),a
    add a,8
    ld (pre2+1),a
    ld (pre2Normal+1),a
    sub 24
    ld (pre3+1),a
    ld (pre3Normal+1),a
    add a,8
    ld (pre4+1),a
    ld (pre4Normal+1),a
    ld hl,columnsPreNormal
    ld de,columnsPre
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
route: jp columnsRaw
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
columnsPreNormal:
    ld hl,(codes+0)
    ld c,h
pre0Normal: ld h,0
pre1Normal: ld b,0
    ld a,(bc)
    or (hl)
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
    ld (de),a
    ld a,e
    sub 7
    ld e,a
    inc d
    ld hl,(codes+1)
    ld c,h
pre2Normal: ld h,0
pre3Normal: ld b,0
    ld a,(bc)
    or (hl)
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
    ld (de),a
    ld a,e
    sub 7
    ld e,a
    inc d
    ld hl,(codes+2)
    ld c,h
pre4Normal: ld h,0
    ld b,FONT_ADDR >> 8
    ld a,(bc)
    or (hl)
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
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
columnsPre:
    ld hl,(codes+0)
    ld c,h
pre0: ld h,0
pre1: ld b,0
    ld a,(bc)
    or (hl)
invPre0_0: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
invPre0_1: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
invPre0_2: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
invPre0_3: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
invPre0_4: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
invPre0_5: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
invPre0_6: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
invPre0_7: cpl
    ld (de),a
    ld a,e
    sub 7
    ld e,a
    inc d
    ld hl,(codes+1)
    ld c,h
pre2: ld h,0
pre3: ld b,0
    ld a,(bc)
    or (hl)
invPre1_0: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
invPre1_1: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
invPre1_2: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
invPre1_3: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
invPre1_4: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
invPre1_5: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
invPre1_6: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
invPre1_7: cpl
    ld (de),a
    ld a,e
    sub 7
    ld e,a
    inc d
    ld hl,(codes+2)
    ld c,h
pre4: ld h,0
    ld b,FONT_ADDR >> 8
    ld a,(bc)
    or (hl)
invPre2_0: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
invPre2_1: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
invPre2_2: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
invPre2_3: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
invPre2_4: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
invPre2_5: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
invPre2_6: cpl
    ld (de),a
    inc b
    inc h
    inc e
    ld a,(bc)
    or (hl)
invPre2_7: cpl
    ld (de),a
    jp done
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
; KOI-7 — перестановка блоков 60h и E0h при переключении режима.
; Вызывается только явно: ни скаляр, ни пакет не проверяет режим KOI.
; A=0 KOI8, иначе KOI7. Все регистры и флаги сохранены.
setEncoding:
    push af
    or a
    jp z,encodingNormalized
    ld a,0FFh
encodingNormalized:
    ld (bios_upper.koi7),a
    call applyEncoding
    pop af
    ret
applyEncoding:
    push af
    ld a,(bios_upper.koi7)
koiOld: cp 0
    jp z,koiQuickDone
    push bc
    push de
    push hl
    ld (koiOld+1),a
    ld h,FONT_ADDR >> 8
    ld b,8
    call swapPages
    call preValid
    jp z,koiDone
    ld a,(preBase+1)
    ld h,a
    ld b,40
    call swapPages
koiDone:
    pop hl
    pop de
    pop bc
koiQuickDone:
    pop af
    ret
swapPages:
    ld d,h
    ld l,60h
    ld e,0E0h
    ld c,32
swapBytes:
    ld a,(de)
    ld (swapSaved+1),a
    ld a,(hl)
    ld (de),a
swapSaved: ld a,0
    ld (hl),a
    inc l
    inc e
    dec c
    jp nz,swapBytes
    inc h
    dec b
    jp nz,swapPages
    ret
preValid:
    ld hl,(preBase)
    ld a,h
    or a
    ret z
    ld hl,(bios_upper.charGen)
    ld a,h
    cp FONT_ADDR >> 8
    jp nz,preStale
    ld a,l
    or a
    jp nz,preStale
    ld a,(preEpoch)
    ld hl,GLYPH_CACHE_EPOCH
    cp (hl)
    jp nz,preStale
    ld a,1
    or a
    ret
preStale:
    xor a
    ret
; FONT_RENDER: предоставленный main-буфер 10 КБ, выровнен на 256.
service:
    or a
    jp z,query
    dec a
    jp z,attach
    dec a
    jp nz,bad
    ld hl,0
    ld (preBase),hl
    jp query
attach:
    ld a,l
    or a
    jp nz,bad
    ld a,h
    or a
    jp z,bad
    cp 65h
    jp nc,bad
    push hl
    ld hl,(bios_upper.charGen)
    ld a,h
    cp FONT_ADDR >> 8
    jp nz,badPop
    ld a,l
    or a
    jp nz,badPop
    pop hl
    ld (preBase),hl
    ld a,(GLYPH_CACHE_EPOCH)
    ld (preEpoch),a
    call buildAll
query:
    call preValid
    push af
    ld hl,(bios_upper.charGen)
    ex de,hl
    ld hl,(preBase)
    ld bc,FONT_PRERENDER_BYTES
    pop af
    or a
    ret
badPop:
    pop hl
bad:
    ld a,DOS_ERR_ARGUMENT
    scf
    ret
; Пять плоскостей подряд; выбор сдвига вынесен из побайтового цикла.
buildAll:
    ld hl,(preBase)
    ex de,hl
    ld hl,FONT_ADDR
    ld b,8
buildPage0:
buildLoop0:
    ld a,(hl)
    add a,a
    add a,a
    ld (de),a
    inc l
    inc e
    jp nz,buildLoop0
    inc h
    inc d
    dec b
    jp nz,buildPage0
    ld hl,FONT_ADDR
    ld b,8
buildPage1:
buildLoop1:
    ld a,(hl)
    rrca
    rrca
    and 0Fh
    ld (de),a
    inc l
    inc e
    jp nz,buildLoop1
    inc h
    inc d
    dec b
    jp nz,buildPage1
    ld hl,FONT_ADDR
    ld b,8
buildPage2:
buildLoop2:
    ld a,(hl)
    rrca
    rrca
    and 0C0h
    ld (de),a
    inc l
    inc e
    jp nz,buildLoop2
    inc h
    inc d
    dec b
    jp nz,buildPage2
    ; b>>4 из готовой плоскости b>>2: ещё два вращения.
    ld hl,(preBase)
    ld a,h
    add a,8
    ld h,a
    ld b,8
buildPage3:
buildLoop3:
    ld a,(hl)
    rrca
    rrca
    and 3
    ld (de),a
    inc l
    inc e
    jp nz,buildLoop3
    inc h
    inc d
    dec b
    jp nz,buildPage3
    ; b<<4 из готовой плоскости b<<2: ещё два сложения.
    ld hl,(preBase)
    ld b,8
buildPage4:
buildLoop4:
    ld a,(hl)
    add a,a
    add a,a
    ld (de),a
    inc l
    inc e
    jp nz,buildLoop4
    inc h
    inc d
    dec b
    jp nz,buildPage4
    ret
buildAllEnd:
realMode: DB 0
produced: DB 0
text: DW 0
codes: DS 4
preBase: DW 0
preEpoch: DB 0
    ENDMODULE
