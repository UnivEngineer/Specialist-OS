; Поле 8 + разделитель + 3: 12 знаков, 6 байт в 4x8, 9 байт в 6x8.
; HL=FILE_INFO; HL/BC/DE сохранены, AF разрушается.
printFileNamOnPanel:
        push hl
        push bc
        push de
        ld de,ncTextBuffer
        ld b,8
        call ncCopyNamePart
        ld a,' '
        ld (de),a
        push hl
        inc hl
        inc hl
        inc hl
        ld a,(hl)
        and 010h
        pop hl
        jp z,printFileGap
        ; Корневой псевдоэлемент уже содержит обратную косую черту.
        push hl
        ld bc,-8
        add hl,bc
        ld a,(hl)
        pop hl
        cp 05Ch
        jp z,printFileGap
        ld a,05Ch
        ld (ncTextBuffer+8),a
printFileGap:
        ld de,ncTextBuffer+9
        ld b,3
        call ncCopyNamePart
        xor a
        ld (de),a
        ld hl,ncTextBuffer
        ld a,(ncFontWidth)
        cp 8
        jp z,ncPrintDetailedFile
        call bios_printStringReal
    IF !ENABLE_COLOR
        ld a,(ncFontWidth)
        cp 6
        call z,ncNamePadding
    ENDIF
ncPanelNameDone:
        pop de
        pop bc
        pop hl
        ret
ncPrintDetailedFile:
        pop de
        pop bc
        pop hl
        push hl
        push bc
        push de
        push hl
        ld de,FILE_INFO.attrib
        add hl,de
        ld a,(hl)
        and 10h
        pop hl
        jp nz,ncDetailedDirectory
        ld de,FILE_INFO.size
        add hl,de
        ld e,(hl)
        inc hl
        ld d,(hl)
        ld b,20h
        call ncFormatDecimal
        jp ncDetailedSuffix
ncDetailedDirectory:
        ld hl,ncDirectory5
ncDetailedSuffix:
        ld de,ncTextBuffer+12
        ld b,5
        ld a,' '
ncDetailedGap:
        ld (de),a
        inc de
        dec b
        jp nz,ncDetailedGap
        ld b,5
        call ncCopyNamePart
        xor a
        ld (de),a
        ld hl,ncTextBuffer
        call bios_printStringReal
        jp ncPanelNameDone
ncDirectory5: DB '<DIR>'
    IF !ENABLE_COLOR
; The 1px right border follows the inverse cursor in monochrome.
ncNamePadding:
        ld hl,(bios_vars.cursorY)
        ld a,h
        ld h,0
        ld d,l
        ld l,a
        add hl,hl
        ld a,d
        sub 8
        ld d,a
        ld e,10
        ld bc,1
        ld a,(bios_vars.inverse)
        jp uiFillRectPixels
    ENDIF
; Поля FAT не являются ASCIIZ; непечатный байт заменяется пробелом.
ncCopyNamePart:
        ld a,(hl)
        cp 20h
        jp nc,ncCopyNameByte
        ld a,' '
ncCopyNameByte:
        ld (de),a
        inc hl
        inc de
        dec b
        jp nz,ncCopyNamePart
        ret
