; HL=FILE_COPY_REQUEST; A=0/1/2/3 (No/Yes/Yes all/No all), CF=0, все регистры разрушены.
cliOverwritePrompt:
    ld de,FILE_COPY_REQUEST.destination
    add hl,de
    push hl
    ld hl,cliOverwriteText
    call bios_printString
    pop hl
    ld b,8
    call cliOverwritePart
    ld a,(hl)
    cp ' '
    jp z,cliOverwriteSuffix
    push hl
    ld c,'.'
    call bios_printChar
    pop hl
    ld b,3
    call cliOverwritePart
cliOverwriteSuffix:
    ld hl,cliOverwriteKeys
    call bios_printString
cliOverwriteKey:
    call bios_getch
    cp 0Dh
    jp z,cliOverwriteNo
    cp 01Bh
    jp z,cliOverwriteNo
    and 0DFh
    cp 'N'
    jp z,cliOverwriteNo
    cp 'A'
    jp z,cliOverwriteAll
    cp 'S'
    jp z,cliOverwriteSkipAll
    cp 'Y'
    jp nz,cliOverwriteKey
    ld a,1
    jp cliOverwriteReturn
cliOverwriteAll:
    ld a,2
    jp cliOverwriteReturn
cliOverwriteSkipAll:
    ld a,3
    jp cliOverwriteReturn
cliOverwriteNo:
    xor a
cliOverwriteReturn:
    push af
    ld c,0Ah
    call bios_printChar
    pop af
    or a
    ret
cliOverwritePart:
    ld a,(hl)
    cp ' '
    jp z,cliOverwritePartNext
    push hl
    push bc
    ld c,a
    call bios_printChar
    pop bc
    pop hl
cliOverwritePartNext:
    inc hl
    dec b
    jp nz,cliOverwritePart
    ret
cliOverwriteText: DB 'Overwrite ',0
cliOverwriteKeys: DB '? Yes/No/All/Skip all [N] ',0
