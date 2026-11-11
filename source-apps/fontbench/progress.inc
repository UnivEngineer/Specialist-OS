; Очищаем верхние 240 строки, нижняя строка результата остаётся видимой.
; Все операции здесь выполняются вне измеряемого интервала.
matrixClearText:
    ld hl,09000h
    ld b,48
matrixClearColumn:
    ld c,FONT_ROWS*10
matrixClearByte:
    ld (hl),0
    inc l
    dec c
    jp nz,matrixClearByte
    ld l,0
    inc h
    dec b
    jp nz,matrixClearColumn
    ret

matrixProgress:
    ; Без паузы и печати 48 пробелов: очищаем только строку 248..255.
    ld hl,090F8h
    ld b,48
matrixProgressClear:
    ld c,8
matrixProgressClearRow:
    ld (hl),0
    inc l
    dec c
    jp nz,matrixProgressClearRow
    ld l,0F8h
    inc h
    dec b
    jp nz,matrixProgressClear
    ld hl,000FFh
    call bios_setCursorPos
    ld hl,matrixStage
    inc (hl)
    ld a,(hl)
    call matrixProgressNumber
    ld c,'/'
    call bios_printChar
    ld a,(benchStageTotal)
    call matrixProgressNumber
    ld c,' '
    call bios_printChar
    ld a,(matrixBank)
    or a
    ld hl,matrixMainText
    jp z,matrixProgressBank
    ld hl,matrixDosText
matrixProgressBank:
    call puts
    ld a,(matrixKind)
    ld hl,matrixFont6Text
    cp 3
    jp c,matrixProgressFont
    ld hl,matrixFont8Text
    cp 5
    jp c,matrixProgressFont
    ld hl,matrixFont4Text
matrixProgressFont:
    call puts
    ld hl,(matrixCase)
    ld de,12
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    call puts
    ld c,' '
    call bios_printChar
    ld hl,v_elapsed
    call print_time_ms
    ld hl,matrixMsText
    jp puts

; Десятичный номер 0..255 без ведущих нулей. Разрушает AF/BC/DE.
matrixProgressNumber:
    ld d,0
    ld b,'0'
matrixProgressHundreds:
    cp 100
    jp c,matrixProgressHundredDone
    sub 100
    inc b
    jp matrixProgressHundreds
matrixProgressHundredDone:
    push af
    ld a,b
    cp '0'
    jp z,matrixProgressNoHundreds
    ld c,b
    call bios_printChar
    ld d,1
matrixProgressNoHundreds:
    pop af
    ld b,'0'
matrixProgressTens:
    cp 10
    jp c,matrixProgressUnits
    sub 10
    inc b
    jp matrixProgressTens
matrixProgressUnits:
    push af
    ld a,d
    or a
    jp nz,matrixProgressShowTens
    ld a,b
    cp '0'
    jp z,matrixProgressNoTens
matrixProgressShowTens:
    ld c,b
    call bios_printChar
matrixProgressNoTens:
    pop af
    add a,'0'
    ld c,a
    jp bios_printChar

matrixMainText: DB 'MAIN ',0
matrixDosText: DB 'BANK ',0
matrixFont6Text: DB '6x8 ',0
matrixFont8Text: DB '8x8 ',0
matrixFont4Text: DB '4x8 ',0
matrixMsText: DB ' ms',0
matrixStage: DB 0
