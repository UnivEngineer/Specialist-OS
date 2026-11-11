; Модальный выбор замены: HL=FILE_COPY_REQUEST, A=0/1/2/3, CF=0.
; 0=No, 1=Yes, 2=Yes to all, 3=No to all. Escape равен No.
; Стрелки/Tab меняют выбор; Y/N/A/S выбирают ответ напрямую.
; Все регистры разрушены. Дисковые службы из обработчика не вызываются.
ncOverwritePrompt:
    push hl
    call ncDamageBoth
    pop hl
    ld de,FILE_COPY_REQUEST.destination
    add hl,de
    ld de,ncOverwriteName
    call compactName
    ld hl,ncOverwriteName
    ld b,0
ncOverwriteTrim:
    ld a,(hl)
    cp 0Dh
    jp z,ncOverwriteTrimEnd
    inc b
    inc hl
    jp ncOverwriteTrim
ncOverwriteTrimEnd:
    dec hl
    ld (hl),0
    dec b
    push bc
    COLOR COLOR_DIALOG
    ld hl,ncOverwriteWindow
    call draw
    ; Y текстового BIOS задаёт нижнюю линию восьмипиксельного знака.
    ld hl,04C6Dh
    ld (bios_vars.cursorY),hl
    ld hl,ncOverwriteTitle
    call bios_printStringReal
    pop bc
    ; Имя центрируется по фактической длине, без пробельного хвоста 8.3.
    ld a,b
    add a,a
    add a,b
    or a
    rra
    ld b,a
    ld a,96
    sub b
    add a,2
    and 0FCh
    ld h,a
    ld l,130
    ld (bios_vars.cursorY),hl
    ld hl,ncOverwriteName
    call bios_printStringReal
    ld hl,02C95h
    ld (bios_vars.cursorY),hl
    ld hl,ncOverwriteYes
    call bios_printStringReal
    ld hl,04095h
    ld (bios_vars.cursorY),hl
    ld hl,ncOverwriteYesAll
    call bios_printStringReal
    ld hl,06C95h
    ld (bios_vars.cursorY),hl
    ld hl,ncOverwriteNo
    call bios_printStringReal
    ld hl,07C95h
    ld (bios_vars.cursorY),hl
    ld hl,ncOverwriteNoAll
    call bios_printStringReal
    ld a,2
    ld (ncOverwriteChoice),a
    call ncOverwriteInvert
ncOverwriteKey:
    call ncModalGetch
    cp 01Bh
    jp z,ncOverwriteNoKey
    cp 0Dh
    jp z,ncOverwriteDone
    cp 8
    jp z,ncOverwriteLeft
    cp 018h
    jp z,ncOverwriteRight
    cp 9
    jp z,ncOverwriteRight
    and 0DFh
    cp 'Y'
    jp z,ncOverwriteYesKey
    cp 'N'
    jp z,ncOverwriteNoKey
    cp 'A'
    jp z,ncOverwriteYesAllKey
    cp 'S'
    jp z,ncOverwriteNoAllKey
    jp ncOverwriteKey
ncOverwriteLeft:
    ld b,0FFh
    jp ncOverwriteChange
ncOverwriteRight:
    ld b,1
ncOverwriteChange:
    push bc
    call ncOverwriteInvert
    pop bc
    ld a,(ncOverwriteChoice)
    add a,b
    and 3
    ld (ncOverwriteChoice),a
    call ncOverwriteInvert
    jp ncOverwriteKey
ncOverwriteYesKey:
    ld a,1
    or a
    ret
ncOverwriteYesAllKey:
    ld a,2
    or a
    ret
ncOverwriteNoAllKey:
    ld a,3
    or a
    ret
ncOverwriteNoKey:
    xor a
    ret
ncOverwriteDone:
    ld a,(ncOverwriteChoice)
    ld e,a
    ld d,0
    ld hl,ncOverwriteAnswers
    add hl,de
    ld a,(hl)
    or a
    ret
; Все ячейки имеют одинаковую высоту и границы по экранным байтам.
; При смене выбора инвертируются только прежняя и новая ячейки.
ncOverwriteInvert:
    ld a,(ncOverwriteChoice)
    add a,a
    add a,a
    ld e,a
    ld d,0
    ld hl,ncOverwriteRects
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld c,(hl)
    inc hl
    ld b,(hl)
    ex de,hl
    jp inverseRect
ncOverwriteRects:
    DW 09A8Ch,(4<<8)+12
    DW 09F8Ch,(9<<8)+12
    DW 0AA8Ch,(3<<8)+12
    DW 0AE8Ch,(8<<8)+12
ncOverwriteAnswers: DB 1,2,0,3
ncOverwriteWindow:
    G_WINDOW 80h,64,96,256,64
    G_LINE 80h,68,116,248
    DB 0
ncOverwriteTitle: DB 'Overwrite file?',0
ncOverwriteYes: DB 'Yes',0
ncOverwriteYesAll: DB 'Yes to all',0
ncOverwriteNo: DB 'No',0
ncOverwriteNoAll: DB 'No to all',0
ncOverwriteChoice: DB 2
ncOverwriteName: DS 16,0
