; Используется только до старта матрицы; затем заменяется фазой 4 пререндера.
; HL=ASCIIZ аргументы, CF=ошибка. bios_argNext сохраняет курсор/длину слова.
benchParseArgs:
    ld a,h
    or l
    ret z
benchNextArg:
    call bios_argNext
    jp c,benchArgsDone
    ld (benchArgCursor),hl
    ex de,hl
    ld (benchToken),hl
    ld a,b
    ld (benchTokenLength),a
    ld hl,benchOptionTable
benchOptionCandidate:
    ld a,(hl)
    or a
    jp z,benchArgsBad
    push hl
    ld hl,(benchToken)
    ex de,hl
    pop hl
    push hl
    ld a,(benchTokenLength)
    ld b,a
benchOptionCompare:
    ld a,b
    or a
    jp z,benchOptionEnd
    ld a,(hl)
    or a
    jp z,benchOptionMismatch
    ld a,(de)
    cp 'a'
    jp c,benchOptionUpper
    cp 'z'+1
    jp nc,benchOptionUpper
    sub 020h
benchOptionUpper:
    cp (hl)
    jp nz,benchOptionMismatch
    inc hl
    inc de
    dec b
    jp benchOptionCompare
benchOptionEnd:
    ld a,(hl)
    or a
    jp nz,benchOptionMismatch
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    pop bc
    ld hl,benchOptions
    ld c,e
    ld b,0
    add hl,bc
    ld (hl),d
    ld hl,(benchArgCursor)
    jp benchNextArg
benchOptionMismatch:
    pop hl
benchOptionSkip:
    ld a,(hl)
    inc hl
    or a
    jp nz,benchOptionSkip
    inc hl
    inc hl
    jp benchOptionCandidate
benchArgsDone:
    or a
    ret
benchArgsBad:
    scf
    ret
benchArgCursor: DW 0
benchToken: DW 0
benchTokenLength: DB 0
benchOptionTable:
    DB '-F=4',0,0,4
    DB '-F=6',0,0,1
    DB '-F=8',0,0,2
    DB '-F=ALL',0,0,7
    DB '-P=RAW',0,1,1
    DB '-P=PACK',0,1,2
    DB '-P=DUP',0,1,4
    DB '-P=ALL',0,1,7
    DB '-B=MAIN',0,2,1
    DB '-B=DOS',0,2,2
    DB '-B=BANK',0,2,2
    DB '-B=BOTH',0,2,3
    DB '-C=CHAR',0,3,1
    DB '-C=SERIAL',0,3,2
    DB '-C=WIN256',0,3,4
    DB '-C=WIN20',0,3,4
    DB '-C=FAR48',0,3,8
    DB '-C=FAR1152',0,3,16
    DB '-C=GROUP',0,3,32
    DB '-C=ALL',0,3,63
    DB '-N',0,4,1
    DB '-Q',0,5,1
    DB '-?',0,6,1
    DB '-E=STANDALONE',0,7,1
    DB '-E=KERNEL',0,7,2
    DB '-E=PAGED',0,7,4
    DB '-E=ALL',0,7,7
    DB '-K',0,7,2
    DB 0
benchUsage:
    DB 'FONTBEN [-F=4|6|8|ALL] [-P=RAW|PACK|DUP|ALL]',13,10
    DB ' -B=MAIN|BANK|BOTH (DOS alias: BANK)',13,10
    DB ' -E=STANDALONE|KERNEL|PAGED|ALL (-K: KERNEL)',13,10
    DB ' -C=CHAR|SERIAL|GROUP|WIN20|FAR48|FAR1152|ALL',13,10
    DB ' -N: no file  -Q: auto return  -?: help',13,10,0
benchInvalidText: DB 'Invalid arguments or no matching cases',13,10,0
