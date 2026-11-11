; Поле 8 + разделитель + 3: 12 знаков, три пакета 6x8.
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
        call bios_printStringReal
        call ncNamePadding
        pop de
        pop bc
        pop hl
        ret
; Полные байты полей вне 12 знаков восстанавливаются вместе с именем.
; Это сохраняет выделение и при выборочной перерисовке после прокрутки.
ncNamePadding:
        ld hl,(bios_vars.cursorY)
        ld a,h
        rrca
        rrca
        add a,090h
        ld h,a
        ld a,l
        sub 8
        ld l,a
        ld a,(bios_vars.inverse)
        ld c,a
ncNamePaddingByte:
        push hl
        ld b,10
ncNamePaddingRow:
        ld (hl),c
        inc l
        dec b
        jp nz,ncNamePaddingRow
        pop hl
        ret
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
