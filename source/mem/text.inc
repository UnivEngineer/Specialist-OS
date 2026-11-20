; Доставка строк и коротких пачек в форме inline профиля COMMAND.
memPrint:
    xor a
    ld (memOperation),a
memPrintTyped:
    push af
    ld a,(bankPage)
    or a
    jp z,memPrintNative
    push bc
    push de
    ld (memTextNext),hl
    ld a,(memOperation)
    cp 2
    ld hl,0FFFFh
    jp c,memPrintCount
    ld h,b
    ld l,c
memPrintCount:
    ld (memTextRemaining),hl
memPrintLoop:
    ld hl,(memTextRemaining)
    ld a,h
    or l
    jp z,memPrintDone
    ld b,20
    ld a,h
    or a
    jp nz,memPrintSize
    ld a,l
    cp 20
    jp nc,memPrintSize
    ld b,l
memPrintSize:
    call bankCommandWindow
    ld a,1
    ld (BANK_BUFFER_KIND),a
    ld hl,(memTextNext)
    ld de,BANK_PACKET
memPrintFill:
    ld a,(hl)
memPrintStore:
    ld (de),a
    inc de
    inc hl
    dec b
    jp nz,memPrintFill
memPrintFilled:
    ld (memTextNext),hl
    xor a
    ld (de),a
    ld a,e
    sub BANK_PACKET & 0FFh
    ld c,a
    ld (memBack),a
    ld hl,0
    add hl,sp
    ld (memSavedSP),hl
    ld a,(memOperation)
    ld d,a
    ld a,(bankPage)
    ld b,a
    xor a
    ld hl,BANK_IO_MAIN
    jp BANK_WINDOW
memPrintMain:
    ld hl,(bankServiceSP)
    ld sp,hl
    push bc
    ld a,d
    ld hl,BANK_PACKET
    call bios_textDispatch
    ld de,-BANK_PACKET
    add hl,de
    ld a,l
    pop bc
    ld c,a
    ld a,2
    ld (BANK_BUFFER_KIND),a
    ld a,b
    dec a
    ld hl,BANK_IO_RAM
    jp BANK_WINDOW
memPrintReturn:
    ld hl,(memSavedSP)
    ld sp,hl
    ld a,(memBack)
    sub c
    jp nz,memPrintPartial
    ld a,(memOperation)
    cp 2
    jp c,memPrintLoop
    ld hl,(memTextRemaining)
    ld b,0
    ld a,l
    sub c
    ld l,a
    ld a,h
    sbc a,b
    ld h,a
    ld (memTextRemaining),hl
    jp memPrintLoop
memPrintPartial:
    ld hl,(memTextNext)
memPrintBack:
    dec hl
    dec a
    jp nz,memPrintBack
    jp memPrintHaveHL
memPrintDone:
    ld hl,(memTextNext)
memPrintHaveHL:
    pop de
    pop bc
    pop af
    or a
    ret
memPrintNative:
    pop af
    jp bios_printStringZ
memTextRemaining: DW 0
memSavedSP: DW 0
memTextNext: DW 0
memOperation: DB 0
memSelect: DB 0
memBack: DB 0
