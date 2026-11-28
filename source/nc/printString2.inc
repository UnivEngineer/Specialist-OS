; Печать поля ASCIIZ с пробельным заполнением одним вызовом BIOS.
; HL=строка, B=ширина 1..96. AF/BC/DE/HL сохранены.
ncPrintField:
        push af
        push bc
        push de
        push hl
        ld de,ncTextBuffer
ncFieldCopy:
        ld a,(hl)
        or a
        jp z,ncFieldPad
        inc hl
        cp 20h
        jp nc,ncFieldByte
        ld a,' '
ncFieldByte:
        ld (de),a
        inc de
        dec b
        jp nz,ncFieldCopy
        jp ncFieldDone
ncFieldPad:
        ld a,' '
        ld (de),a
        inc de
        dec b
        jp nz,ncFieldPad
ncFieldDone:
        xor a
        ld (de),a
        ld hl,ncTextBuffer
        call FONT_ENGINE_RAWZ
        pop hl
        pop de
        pop bc
        pop af
        ret

; Один добавленный знак редактора; не перерисовывать ради него всё поле.
; C=знак. AF/BC/DE/HL сохранены.
ncPrintChar:
        push af
        push hl
        ld a,c
        ld (ncCharText),a
        ld hl,ncCharText
        call FONT_ENGINE_RAWZ
        pop hl
        pop af
        ret
ncCharText:     DB     0,0
ncTextBuffer:   BLOCK  97,0

; HL=ASCIIZ, B=maximum characters (1..96). Never wraps or pads the field.
; AF/BC/DE preserved; HL points after the printed prefix (NUL or truncation).
ncPrintTextLim:
        push af
        push bc
        push de
        push hl
        ld e,b
        ld bc,0
ncTextLimCount:
        ld a,(hl)
        or a
        jp z,ncTextLimReady
        inc hl
        inc c
        dec e
        jp nz,ncTextLimCount
ncTextLimReady:
        pop hl
        call FONT_ENGINE_LIM
        pop de
        pop bc
        pop af
        ret

; Bounded information-panel text. Like printString: HL ends on the ORIGINAL
; terminator, BC/DE/AF preserved (the driver description follows that NUL).
; Padding is deliberately omitted: adjacent values retain their own color.
ncPrintPanelText:
        push af
        push bc
        push de
        push hl
        ld a,(mon2_vars.cursorX)
        cp 96
        jp c,ncPanelTextLeft
        sub 96
ncPanelTextLeft:
        ld c,a
        ld a,92
        sub c
        jp c,ncPanelTextZero
        ld c,a
        ld a,(ncFontWidth)
        rrca
        ld e,a
        ld a,c
        ld b,0
ncPanelTextCount:
        sub e
        jp c,ncPanelTextCopyStart
        inc b
        jp ncPanelTextCount
ncPanelTextZero:
        ld b,0
ncPanelTextCopyStart:
        pop hl
        ld de,ncTextBuffer
ncPanelTextCopy:
        ld a,(hl)
        or a
        jp z,ncPanelTextEnd
        inc hl
        ld c,a
        ld a,b
        or a
        jp z,ncPanelTextCopy
        ld a,c
        ld (de),a
        inc de
        dec b
        jp ncPanelTextCopy
ncPanelTextEnd:
        xor a
        ld (de),a
        push hl
        ld hl,ncTextBuffer
        call FONT_ENGINE_RAWZ
        pop hl
        pop de
        pop bc
        pop af
        ret

; DE=число, B=маска ведущих нулей, как у bios_printDecWord.
; Регистры BC/DE/HL сохранены. Разложение — штатный алгоритм BIOS,
; но пять цифр передаются рендереру одной строкой.
ncPrintDecWord:
        push bc
        push de
        push hl
        call ncFormatDecimal
        call FONT_ENGINE_RAWZ
        pop hl
        pop de
        pop bc
        ret
ncFormatDecimal:
        ex de,hl
        ld a,0
        ld (ncDecimalCount),a
        ld de,-10000
        call ncDecimalDigit
        ld de,-1000
        call ncDecimalDigit
        ld de,-100
        call ncDecimalDigit
        ld de,-10
        call ncDecimalDigit
        ld a,l
        add a,'0'
        call ncDecimalStore
        ld hl,ncDecimalText
        ret
ncDecimalDigit:
        ld c,'0'-1
ncDecimalSubtract:
        inc c
        add hl,de
        jp c,ncDecimalSubtract
        ld a,d
        cpl
        ld d,a
        ld a,e
        cpl
        ld e,a
        inc de
        add hl,de
        ld a,c
        cp '0'
        jp z,ncDecimalMask
        ld b,0FFh
ncDecimalMask:
        and b
        ret z
ncDecimalStore:
        push hl
        push de
        push bc
        ld e,a
        ld a,(ncDecimalCount)
        ld c,a
        inc a
        ld (ncDecimalCount),a
        ld b,0
        ld hl,ncDecimalText
        add hl,bc
        ld (hl),e
        inc hl
        ld (hl),0
        pop bc
        pop de
        pop hl
        ret
;----------------------------------------------------------------------------
; MXOS
;
; Форматировать размер файла в поле до пяти знаков. До 65535 выводятся байты.
; Далее выводятся целые КиБ (K), с 10000 КиБ — целые МиБ (M), с округлением вниз.
;
; вход:
;   HL = FILE_INFO в текущей странице, размер хранится в четырёх байтах.
;
; выход:
;   HL = ASCIIZ в ncDecimalText, ncDecimalCount = длина без нуля.
;   Буфер действителен до следующего форматирования числа. AF, BC, DE и флаги разрушены.
;----------------------------------------------------------------------------
ncFormatFileSize:
        ld de,FILE_INFO.size
        add hl,de
        ld e,(hl)
        inc hl
        ld d,(hl)
        inc hl
        ld c,(hl)
        inc hl
        ld b,(hl)
        ld a,b
        or c
        jp z,ncFormatDecimal

        ; 10000 КиБ = 009C4000h. Младший байт при делении на 1024 не нужен.
        ld a,b
        or a
        jp nz,ncFileSizeMiB
        ld a,c
        cp 09Ch
        jp c,ncFileSizeKiB
        jp nz,ncFileSizeMiB
        ld a,d
        cp 040h
        jp nc,ncFileSizeMiB
ncFileSizeKiB:
        ld e,d
        ld d,c
        ld bc,2*256+'K'
        jp ncFileSizeShift
ncFileSizeMiB:
        ; Для деления на 1048576 достаточно старшего слова и сдвига на 4.
        ld e,c
        ld d,b
        ld bc,4*256+'M'
ncFileSizeShift:
        xor a
        ld a,d
        rra
        ld d,a
        ld a,e
        rra
        ld e,a
        dec b
        jp nz,ncFileSizeShift
        push bc
        call ncFormatDecimal
        pop bc
        ld a,c
        jp ncDecimalStore

ncDecimalCount:     DB     0
ncDecimalText:      BLOCK  6,0
