; Восемь независимых кадров: FILE_HANDLE, команда, номер/смещение строки
; и 512 байт чтения. Дочерний BAT возвращается обычным RET в родителя.
BAT_POS = 64
BAT_LEFT = 66
BAT_LINE = 68
BAT_OFFSET = 70             ; DWORD: начало текущей строки в файле
BAT_CR = 74
BAT_COMMAND = 76
BAT_INPUT = 256
    ASSERT BAT_COMMAND+MAX_PATH+2 <= BAT_INPUT
    ASSERT BAT_INPUT+BAT_READ_SIZE <= BAT_FRAME_SIZE

execBat:
    pop de                  ; Аргументы, положенные загрузчиком.
    IFDEF BANKED_DOS_BUILD
    ld a,(moduleDosBorrowed)
    or a
    jp z,batArenaFree
    ld a,DOS_ERR_ACCESS
    jp dosSetError
batArenaFree:
    ENDIF
    ld a,(v_batDepth)
    cp BAT_MAX_DEPTH
    jp nc,blockArgument
    or a
    jp nz,batNested
    xor a
    ld (v_batReported),a
batNested:
    inc a
    ld (v_batDepth),a
    ld hl,(v_batFrame)
    push hl
    ld de,BAT_FRAME_SIZE
    add hl,de
    ld (v_batFrame),hl
    ex de,hl
    ld b,BAT_COMMAND
    xor a
    call memset_de_a_b
    ld hl,(v_batFrame)
    ld b,h
    ld c,l
    ld hl,v_curFileDescr
    ld de,v_curFileDescr+11
    call memcpy_bc_hl
    ld a,FH_DRIVE
    call batField
    ld a,(v_drive)
    ld (hl),a
    inc hl
    ld (hl),FILE_MODE_READ
    ld hl,(v_batFrame)
    ld de,BAT_INPUT
    add hl,de
    ex de,hl
    ld a,FH_BUFFER
    call batField
    ld (hl),e
    inc hl
    ld (hl),d
    xor a
    call batIo
    call nc,batRun
    push af
    ld a,FH_STATE
    call batField
    ld a,(hl)
    cp 046h
    jp nz,batNoClose
    ld a,FILE_OP_CLOSE
    call batIo
    push af
    pop de
    pop af
    jp c,batClosed
    push de
    pop af
    jp batClosed
batNoClose:
    pop af
batClosed:
    call c,batError
    push af
    ld hl,v_batDepth
    dec (hl)
    pop bc
    pop hl
    ld (v_batFrame),hl
    push bc
    pop af
    ret

; A=смещение поля, HL=его адрес; BC/DE/A сохраняются.
batField:
    push de
    ld hl,(v_batFrame)
    ld e,a
    ld d,0
    add hl,de
    pop de
    ret

; Локальный FILE_HANDLE: восстановить диск и временный каталог команды.
batIo:
    ld c,a
    ld a,(v_drive)
    push af
    ld hl,(v_directory)
    push hl
    ld a,c
    ld hl,(v_batFrame)
    or a
    jp z,batIoOpen
    call fileHandle
    jp batIoDone
batIoOpen:
    ; Имя уже разрешено загрузчиком: OPEN не должен сбросить временный
    ; каталог пути A:\DIR\FILE.BAT к сохранённому CWD этого диска.
    ld (v_handle),hl
    ld (v_handleOp),a
    ld hl,0
    ld (v_handleNew),hl
    call dosClearError
    call handleOpen
batIoDone:
    push af
    pop bc
    pop hl
    pop af
    push hl
    push bc
    call fileSelectDrive
    push af
    pop de
    pop bc
    pop hl
    ld (v_directory),hl
    push bc
    pop af
    ret c
    push de
    pop af
    ret

; CF=0/A=байт; CF=1/A=0 — EOF; CF=1/A!=0 — ошибка. BC/DE/HL сохранены.
batByte:
    push hl
    push de
    push bc
    ld a,BAT_LEFT
    call batField
    ld e,(hl)
    inc hl
    ld d,(hl)
    ld a,d
    or e
    jp nz,batByteReady
    ld a,FH_COUNT
    call batField
    ld (hl),BAT_READ_SIZE & 0FFh
    inc hl
    ld (hl),BAT_READ_SIZE / 256
    ld a,FILE_OP_READ
    call batIo
    jp c,batByteExit
    ld a,FH_COUNT
    call batField
    ld e,(hl)
    inc hl
    ld d,(hl)
    ld a,BAT_LEFT
    call batField
    ld (hl),e
    inc hl
    ld (hl),d
    ld a,d
    or e
    scf
    jp z,batByteExit
    ld hl,(v_batFrame)
    ld de,BAT_INPUT
    add hl,de
    ex de,hl
    ld a,BAT_POS
    call batField
    ld (hl),e
    inc hl
    ld (hl),d
    ld a,BAT_LEFT
    call batField
    ld e,(hl)
    inc hl
    ld d,(hl)
batByteReady:
    dec de
    ld (hl),d
    dec hl
    ld (hl),e
    dec hl
    ld d,(hl)
    dec hl
    ld e,(hl)
    ld a,(de)
    inc de
    ld (hl),e
    inc hl
    ld (hl),d
    or a
batByteExit:
    pop bc
    pop de
    pop hl
    ret

batRun:
    call batByte
    jp c,batEnd
    push af
    ld a,BAT_CR
    call batField
    ld c,(hl)
    ld (hl),0
    pop af
    cp 0Ah
    jp nz,batFirst
    dec c
    jp z,batRun             ; LF после CR не создаёт вторую строку.
batFirst:
    push af
    ld a,BAT_LINE
    call batField
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc de
    ld a,d
    or e
    jp nz,batLineReady
    pop af
    jp blockArgument        ; Не допускать незаметного переполнения номера строки.
batLineReady:
    ld (hl),d
    dec hl
    ld (hl),e
    call batOffset
    ld a,BAT_COMMAND
    call batField
    ld b,MAX_PATH-1
    pop af
batCharacter:
    cp 0Dh
    jp z,batTerminator
    cp 0Ah
    jp z,batTerminator
    or a
    jp z,blockArgument      ; NUL внутри файла не является EOF.
    ld (hl),a
    inc hl
    dec b
    jp z,batTooLong
    call batByte
    jp nc,batCharacter
    or a
    jp nz,batFail
    xor a                   ; Последняя строка без CR/LF.
batTerminator:
    ld (hl),0
    push af
    ld a,BAT_CR
    call batField
    pop af
    cp 0Dh
    jp nz,batCommand
    ld (hl),1
batCommand:
    ld a,BAT_COMMAND
    call batField
    ld a,(hl)
    or a
    jp z,batRun
    cp '@'
    jp nz,batEcho
    inc hl
    jp batExecute
batEcho:
    ld a,(v_batEcho)
    or a
    jp z,batExecute
    push hl
    call j_printString
    call printCharA
    pop hl
batExecute:
    call fileExec
    ret c
    jp batRun
batTooLong:
    call batByte
    jp c,batLongEnd
    cp 0Dh
    jp z,batTerminator
    cp 0Ah
    jp z,batTerminator
    jp blockArgument
batLongEnd:
    or a
    jp nz,batFail
    jp batTerminator
batEnd:
    or a
    ret z
batFail:
    scf
    ret

; Первый байт уже прочитан: смещение строки = FH_POSITION - LEFT - 1.
batOffset:
    ld a,FH_POSITION
    call batField
    ex de,hl
    ld a,BAT_OFFSET
    call batField
    ld b,4
batOffsetCopy:
    ld a,(de)
    ld (hl),a
    inc de
    inc hl
    dec b
    jp nz,batOffsetCopy
    ld a,BAT_LEFT
    call batField
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc de
    ld a,BAT_OFFSET
    call batField
    ld a,(hl)
    sub e
    ld (hl),a
    inc hl
    ld a,(hl)
    sbc a,d
    ld (hl),a
    inc hl
    ld a,(hl)
    sbc a,0
    ld (hl),a
    inc hl
    ld a,(hl)
    sbc a,0
    ld (hl),a
    ret

batError:
    call dosSetError
    push af
    ld a,(v_batReported)
    or a
    jp nz,batErrorDone
    inc a
    ld (v_batReported),a
    ld hl,batErrorText
    call j_printString
    ld hl,(v_batFrame)
    ld b,8
    call batErrorName
    ld c,'.'
    call j_printChar
    ld b,3
    call batErrorName
    ld hl,batLineText
    call j_printString
    ld a,BAT_LINE
    call batField
    ld e,(hl)
    inc hl
    ld d,(hl)
    ld b,0
    call bios_printDecWord
    ld hl,batCodeText
    call j_printString
    pop af
    push af
    ld e,a
    ld d,0
    ld b,0
    call bios_printDecWord
    call printCharA
batErrorDone:
    ld a,(v_batDepth)
    dec a
    jp nz,batErrorReturn
    ld a,(v_bootAutoexec)
    or a
    jp nz,batErrorReturn
    ld a,(v_shell)
    or a
    jp nz,batErrorReturn
batWaitError:
    call j_getch            ; NC иначе сразу сотрёт диагностику при перезагрузке.
batErrorReturn:
    pop af
    ret
batErrorName:
    ld a,(hl)
    inc hl
    cp ' '
    jp z,batErrorNameNext
    ld c,a
    call j_printChar
batErrorNameNext:
    dec b
    jp nz,batErrorName
    ret
batErrorText: DB "BAT ",0
batLineText: DB " LINE ",0
batCodeText: DB ": ERROR ",0
v_batDepth: DB 0
v_batReported: DB 0
v_batFrame: DW BAT_BUFFER-BAT_FRAME_SIZE
