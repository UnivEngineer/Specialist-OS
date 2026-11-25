; Построчный 6x8: полные байты экрана, два указателя на столбец.
; Внутренний вход нерекурсивный, аппаратные IRQ выключены.
    MODULE fast6
; HL=следующий байт текста. realMode: 0=Z, 1=RawZ, 2=RawLim, 80h=Lim.
; В режимах RawLim и Lim BC задаёт доступное число байтов.
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
    add a,a                    ; 0 и 80h обрабатывают ESC.
    jp z,checkEsc
    rrca
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
    ld hl,groupNormalDown
    ld de,groupInverseDown
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
    jp c,endReady
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
    ld (normalDownLimit+1),a
    ld (normalUpLimit+1),a
    ld (inverseDownLimit+1),a
    ld (inverseUpLimit+1),a
    ld a,l
    sub 7
    ld e,a
route: jp groupNormalDown
scanGroup:
    ld hl,(text)
    ; Выбор проверок один раз на пакет. Длина RawLim уже ограничена квотой.
scanRoute:
    jp scan
scanRaw:
    MACRO READ_CODE offset
        ld a,(hl)
        IFDEF FONT_KOI7
        IF K7_MODE == 3 && (offset & 1)
        or 80h
        ELSE
        and 7Fh
        ENDIF
        ENDIF
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
    jp c,abortScan
    IFDEF FONT_KOI7
    and 7Fh
    ENDIF
    ld (bc),a
    inc bc
    inc hl
    ld a,c
    cp (codes+4) & 0FFh
    jp nz,scanByte
    IFDEF FONT_KOI7
    IF K7_MODE == 3
    ; Tag the two right-hand indices once. The middle phase pair has its
    ; halves swapped, so the same tagged codes serve all three columns.
    ld b,h
    ld c,l
    ld hl,codes+1
    ld a,(hl)
    or 80h
    ld (hl),a
    inc hl
    inc hl
    ld a,(hl)
    or 80h
    ld (hl),a
    ld h,b
    ld l,c
    ENDIF
    ENDIF
scanDone:
    ld (text),hl
    ret
abortScan:
    pop hl
    jp noGroup

    MACRO GROUP_FINISH next, limit
    inc d
    ld a,d
limit: cp 0BEh
    jp c,next
    jp noGroup
    ENDM
    IFDEF FONT_KOI7
    INCLUDE "koi7-columns.inc"
    ELSE
    IFDEF FONT_ROTATE_ROWS
    INCLUDE "rotate-rows.inc"
    ELSE
    INCLUDE "packed-columns.inc"
    ENDIF
    ENDIF
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
realMode:       DB 0
text:           DW 0
codes:          DS 4
    ENDMODULE
