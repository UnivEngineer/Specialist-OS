; Аргументы DE при входе COM: [A:..H:] [-C=0..2] [-W=0..9] [-S=1..24] [-?].
; Слова выделяет штатный bios_argNext; исходная строка не изменяется.
; parse_arguments: HL=строка, CF=ошибка, AF/BC/DE/HL разрушены.
; FFh отмечает отсутствующий диск/режим/проходы, ноль — отсутствующий размер.
parse_arguments:
    ld a,0FFh
    ld (argDrive),a
    ld (argCheck),a
    ld (argWrites),a
    xor a
    ld (argSize),a
    ld (argHelp),a
argument_next:
    call bios_argNext
    jp c,arguments_done
    push hl
    ex de,hl
    call argument_word
    pop hl
    ret c
    jp argument_next
arguments_done:
    or a
    ret
argument_word:
    ld a,(hl)
    cp '-'
    jp z,argument_switch
    ld a,b
    cp 2
    jp nz,argument_bad
    ld a,(hl)
    call bios_upperCase
    sub 'A'
    cp 8
    jp nc,argument_bad
    ld c,a
    inc hl
    ld a,(hl)
    cp ':'
    jp nz,argument_bad
    ld a,(argDrive)
    cp 0FFh
    jp nz,argument_bad
    ld a,c
    ld (argDrive),a
    or a
    ret
argument_switch:
    inc hl
    ld a,b
    cp 2
    jp nz,argument_value
    ld a,(hl)
    cp '?'
    jp nz,argument_bad
    ld a,1
    ld (argHelp),a
    or a
    ret
argument_value:
    cp 4
    jp c,argument_bad
    ld a,(hl)
    call bios_upperCase
    ld c,a
    inc hl
    ld a,(hl)
    cp '='
    jp nz,argument_bad
    inc hl
    dec b
    dec b
    dec b
    push bc
    call argument_number
    pop bc
    ret c
    ld e,a
    ld a,c
    cp 'C'
    jp z,argument_check
    cp 'W'
    jp z,argument_writes
    cp 'S'
    jp nz,argument_bad
    ld a,(argSize)
    or a
    jp nz,argument_bad
    ld a,e
    or a
    jp z,argument_bad
    cp MAX_FILE_KIB+1
    jp nc,argument_bad
    ld (argSize),a
    or a
    ret
argument_check:
    ld a,e
    cp 3
    jp nc,argument_bad
    ld hl,argCheck
    jp argument_store
argument_writes:
    ld a,e
    cp 10
    jp nc,argument_bad
    ld hl,argWrites
argument_store:
    ld a,(hl)
    cp 0FFh
    jp nz,argument_bad
    ld (hl),e
    or a
    ret
argument_bad:
    scf
    ret

; HL=цифры, B=длина >0. A=число 0..255, CF=ошибка, BC/DE/HL разрушены.
; Проверка переноса выполняется при каждом сложении, усечения нет.
argument_number:
    ld c,0
argument_digit:
    ld a,(hl)
    sub '0'
    cp 10
    jp nc,argument_bad
    ld d,a
    ld a,c
    add a,a
    ret c
    add a,a
    ret c
    add a,c
    ret c
    add a,a
    ret c
    add a,d
    ret c
    ld c,a
    inc hl
    dec b
    jp nz,argument_digit
    or a
    ret

; A=режим проверки 0/1/2. Установить v_writeMode=1/6/5.
; CF=ошибка/нет DISK_CAP_VERIFY, AF разрушен, остальные регистры сохранены.
select_check:
    or a
    jp z,check_plain
    cp 1
    jp z,check_file
    cp 2
    jp nz,argument_bad
    ld a,(v_caps)
    and DISK_CAP_VERIFY
    jp z,argument_bad
    ld a,5
    jp check_ready
check_file:
    ld a,6
    jp check_ready
check_plain:
    ld a,1
check_ready:
    ld (v_writeMode),a
    or a
    ret

argument_error:
    ld hl,txtArgumentsError
    call puts
argument_help:
    ld hl,txtArgumentsHelp
    jp show_error
txtArgumentsError: DB "Invalid arguments",00Ah,0
txtArgumentsHelp:
    DB "DISKBEN [A:..H:] [-C=0..2] [-W=0..9] [-S=KiB]",00Ah
    DB "-C: 0=plain, 1=file verify, 2=raw sector verify",00Ah
    DB "-W: write/read passes; 0=raw read only",00Ah
    DB "-S: file size in KiB (1-24); -?: help",00Ah
    DB "Missing values are requested interactively.",00Ah,0
v_arguments: DW 0
argDrive: DB 0FFh
argCheck: DB 0FFh
argWrites: DB 0FFh
argSize: DB 0
argHelp: DB 0
