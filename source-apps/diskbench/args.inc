; Аргументы: [A:..H:] [-C:MAIN|DOS|BANK|ALL] [-M:RAW|FILE|ALL] [-V=0..2] [-W=0..9] [-S=1..24].
; Слова выделяет штатный bios_argNext; исходная строка не изменяется.
; parse_arguments: HL=строка, CF=ошибка, AF/BC/DE/HL разрушены.
; FFh отмечает отсутствующий диск/режим/проходы, ноль — отсутствующий размер.
parse_arguments:
    ld a,0FFh
    ld (argDrive),a
    ld (argCheck),a
    ld (argFrag),a
    ld (argWrites),a
    xor a
    ld (argSize),a
    ld (argHelp),a
    ld (argBank),a
    ld (argMode),a
argument_next:
    MXOS_CALL MXOS_BIOS_ARG_NEXT
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
    call bench_upperCase
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
    call bench_upperCase
    ld c,1
    cp 'B'
    jp z,argument_bank
    inc c
    cp 'D'
    jp z,argument_bank
    inc c
    cp 'M'
    jp z,argument_bank
    inc c
    cp 'A'
    jp z,argument_bank
    cp '?'
    jp nz,argument_bad
    ld a,1
    ld (argHelp),a
    or a
    ret
argument_bank:
    ld a,(argBank)
    or a
    jp nz,argument_bad
    ld a,c
    ld (argBank),a
    or a
    ret
; HL=слово, B=точная длина. Имена чувствительны только к ASCII-буквам.
argument_mode:
    ld de,modeNames
    ld a,4
    ld (context_limit+1),a
    ld a,1
    ld (namedMode),a
    jp named_start
argument_context:
    ld de,contextNames
    ld a,5
    ld (context_limit+1),a
    xor a
    ld (namedMode),a
named_start:
    ld c,1
context_next:
    push hl
    push bc
context_match:
    ld a,(de)
    or a
    jp z,context_end
    ld c,a
    ld a,b
    or a
    jp z,context_skip
    ld a,(hl)
    and 0DFh
    cp c
    jp nz,context_skip
    inc hl
    inc de
    dec b
    jp context_match
context_end:
    ld a,b
    or a
    jp nz,context_skip
    pop bc
    pop hl
    ld a,(namedMode)
    or a
    jp z,argument_bank
    ld a,(argMode)
    or a
    jp nz,argument_bad
    ld a,c
    ld (argMode),a
    or a
    ret
context_skip:
    ld a,(de)
    inc de
    or a
    jp nz,context_skip
    pop bc
    pop hl
    inc c
    ld a,c
context_limit:
    cp 5
    jp nz,context_next
    jp argument_bad
namedMode: DB 0
modeNames: DB "RAW",0,"FILE",0,"ALL",0
contextNames: DB "BANK",0,"DOS",0,"MAIN",0,"ALL",0
argument_value:
    cp 4
    jp c,argument_bad
    ld a,(hl)
    call bench_upperCase
    ld c,a
    inc hl
    ld a,(hl)
    cp ':'
    jp nz,argument_equals
    inc hl
    dec b
    dec b
    dec b
    ld a,c
    cp 'C'
    jp z,argument_context
    cp 'M'
    jp z,argument_mode
    jp argument_bad
argument_equals:
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
    cp 'V'
    jp z,argument_check
    cp 'W'
    jp z,argument_writes
    cp 'F'
    jp z,argument_fragment
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
argument_fragment:
    ld a,e
    cp 2
    jp nc,argument_bad
    ld hl,argFrag
    jp argument_store
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
txtArgumentsError: DB "Bad arguments",00Ah,0
txtArgumentsHelp:
    DB "DISKBEN [A:..H:] [-C:MAIN|DOS|BANK|ALL]",10
    DB "-M:RAW|FILE|ALL [ALL]",10
    DB "-V=0/1/2: none/file/sector",10
    DB "-W=0..9 passes [1], 0=read",10
    DB "-S=1..24 KiB [8], DOS/ALL <=8",10
    DB "-F=0/1 seq/rnd; -? help",10
    DB "Missing?",10,0
v_arguments: DW 0
argDrive: DB 0FFh
argCheck: DB 0FFh
argWrites: DB 0FFh
argSize: DB 0
argHelp: DB 0
argBank: DB 0

argFrag: DB 0FFh
argMode: DB 0
