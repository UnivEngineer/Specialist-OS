; ATTRIB [+-R] [+-H] [+-S] [+-A] путь; без масок показывает атрибуты.
; Программный интерфейс — существующий bios_fileGetSetAttr (C857h).
cliAttrib:
    xor a
    ld (cliAttribSet),a
    ld (cliAttribClear),a
    ld hl,(cliArgs)
cliAttribOption:
    call cliSkipSpaces
    ld a,(hl)
    cp '+'
    jp z,cliAttribSign
    cp '-'
    jp nz,cliAttribPath
cliAttribSign:
    ld c,a
    inc hl
cliAttribLetters:
    ld a,(hl)
    and 0DFh
    ld b,1
    cp 'R'
    jp z,cliAttribBit
    ld b,2
    cp 'H'
    jp z,cliAttribBit
    ld b,4
    cp 'S'
    jp z,cliAttribBit
    ld b,020h
    cp 'A'
    jp nz,blockArgument
cliAttribBit:
    ld a,c
    cp '+'
    ld de,cliAttribSet
    jp z,cliAttribMask
    ld de,cliAttribClear
cliAttribMask:
    ld a,(de)
    or b
    ld (de),a
    ; При повторном указании бита действует последняя маска.
    ld a,c
    cp '+'
    ld de,cliAttribClear
    jp z,cliAttribOpposite
    ld de,cliAttribSet
cliAttribOpposite:
    ld a,b
    cpl
    ld b,a
    ld a,(de)
    and b
    ld (de),a
    inc hl
    ld a,(hl)
    cp ' '
    jp z,cliAttribOption
    or a
    jp z,blockArgument
    jp cliAttribLetters
cliAttribPath:
    ld (cliArgs),hl
    call cliOnePath
    ret c
    call cliRemember
    call cliPrepareSource
    jp c,cliFileDone
    ld hl,cliRequest.source
    ld c,0
    call fileGetSetAttr
    jp c,cliFileDone
    ld b,a
    ld a,(cliAttribSet)
    ld c,a
    ld a,(cliAttribClear)
    or c
    jp z,cliAttribPrint
    ld a,(cliAttribClear)
    cpl
    and b
    or c
    ld b,a
    ld c,1
    call cliChanged
    call fileGetSetAttr
    jp cliFileDone
cliAttribPrint:
    ld hl,cliAttribLettersText
    ld de,cliAttribBits
cliAttribPrintLoop:
    ld a,(de)
    and b
    ld c,'-'
    jp z,cliAttribPrintChar
    ld c,(hl)
cliAttribPrintChar:
    call bios_printChar
    inc hl
    inc de
    ld a,(hl)
    or a
    jp nz,cliAttribPrintLoop
    ld c,' '
    call bios_printChar
    ld hl,cliPath1
    call bios_printString
    call cliNewLine
    jp cliFileDone
cliAttribLettersText: DB 'RHSA',0
cliAttribBits: DB 1,2,4,020h
