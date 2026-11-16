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
        call bios_printStringReal
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
        call bios_printStringReal
        pop hl
        pop af
        ret
ncCharText: DB 0,0
ncTextBuffer: BLOCK 97,0

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
        call bios_printStringRawLim
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
        ld a,(bios_vars.cursorX)
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
        call bios_printStringReal
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
        call bios_printStringReal
        pop hl
        pop de
        pop bc
        ret
; Размер справа: три пробела и пять цифр, один выровненный пакет из 8 знаков.
ncPrintFileSize:
        call ncFormatDecimal
        ld hl,ncDecimalPadding
        jp bios_printStringReal
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
ncDecimalCount: DB 0
ncDecimalPadding: DB "   "
ncDecimalText: BLOCK 6,0
