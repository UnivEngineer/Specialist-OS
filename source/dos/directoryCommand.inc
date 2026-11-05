; Общие CD/CHDIR: HL=строка, A=FFh/CF=0 — не встроенная команда.
; Иначе A/CF — результат. BC/DE/HL разрушены. Буфер CMD_LINE общий с CLI.
directoryCommand:
            push hl
            ld de,cdShort
            call cdMatch
            pop hl
            jp z,cdArgument
            push hl
            ld de,cdLong
            call cdMatch
            pop hl
            jp z,cdArgument
            ld a,0FFh
            or a
            ret
cdArgument:
            ; B — длина совпавшей команды.
            ld e,b
            ld d,0
            add hl,de
cdSpaces:
            ld a,(hl)
            cp ' '
            jp nz,cdCopy
            inc hl
            jp cdSpaces
cdCopy:
            cp 021h
            jp c,cdPrint
            ld de,CMD_LINE
            ld b,MAX_PATH
cdCopyLoop:
            ld a,(hl)
            cp 021h
            jp c,cdEnd
            ld (de),a
            inc de
            inc hl
            dec b
            jp nz,cdCopyLoop
            jp blockArgument
cdEnd:
            xor a
            ld (de),a
            ld hl,CMD_LINE
            jp directoryPathUnchecked
cdPrint:
            ld hl,CMD_LINE
            call directoryCwdUnchecked
            ret c
            ld hl,CMD_LINE
            call bios_printString
            xor a
            ret
cdMatch:
            ld b,0
cdMatchLoop:
            ld a,(de)
            or a
            jp z,cdMatchEnd
            ld c,a
            ld a,(hl)
            and 0DFh
            cp c
            ret nz
            inc hl
            inc de
            inc b
            jp cdMatchLoop
cdMatchEnd:
            ld a,(hl)
            cp 021h
            jp c,cdMatched
            or 1
            ret
cdMatched:
            xor a
            ret
cdShort: DB 'CD',0
cdLong: DB 'CHDIR',0
