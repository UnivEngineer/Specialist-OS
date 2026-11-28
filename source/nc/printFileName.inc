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
        call FONT_ENGINE_RAWZ
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
;----------------------------------------------------------------------------
; MXOS
;
; Дополнить имя в 8×8 пятисимвольным полем размера или маркером каталога.
;
; вход:
;   HL = FILE_INFO, первые 12 символов ncTextBuffer содержат имя на панели.
;   В стеке сохранены DE, BC, HL вызывающей программы.
;
; выход:
;   Имя и размер выведены в текущей строке панели. BC, DE, HL восстановлены
;   из стека, AF и флаги разрушены. ncTextBuffer содержит полную строку ASCIIZ.
;----------------------------------------------------------------------------
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
        call ncFormatFileSize
        ld a,(ncDecimalCount)
        ld c,a
        ld a,5
        sub c
        ld de,ncTextBuffer+12
        jp z,ncDetailedCopy
        ld b,a
        ld a,' '
ncDetailedGap:
        ld (de),a
        inc de
        dec b
        jp nz,ncDetailedGap
        jp ncDetailedCopy
ncDetailedDirectory:
        ld hl,ncDirectory5
        ld c,5
        ld de,ncTextBuffer+12
ncDetailedCopy:
        ld b,c
        call ncStatusCopy
        xor a
        ld (de),a
        ld hl,ncTextBuffer
        call FONT_ENGINE_RAWZ
        jp ncPanelNameDone
ncDirectory5: DB '<DIR>'
    IF !ENABLE_COLOR
; The 1px right border follows the inverse cursor in monochrome.
ncNamePadding:
        ld hl,(mon2_vars.cursorY)
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
        ld a,(mon2_vars.inverse)
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
