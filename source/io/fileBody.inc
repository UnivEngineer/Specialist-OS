; Загрузка тела без разбора формата контейнера, только из main.
; HL=FILE_BODY_IO с готовым дескриптором выбранного диска.
; Выход CF/A=ошибка DOS, actual=передано тело; BC/DE/HL/флаги разрушены.
; Заголовок должен помещаться в первый кластер. Хвост необязателен.
; Нереентерабельно: как FILEHANDLE; код, стек и запрос нельзя затирать.
fileBodyLoad:
    ld (fbRequest),hl
    ld de,FILE_BODY_IO.actual
    add hl,de
    ld (hl),0
    inc hl
    ld (hl),0
    ld de,FILE_DESCRIPTOR.size+2
    call fbField
    ld a,(hl)
    inc hl
    or (hl)
    jp nz,fbBad
    dec hl
    dec hl
    ld d,(hl)
    dec hl
    ld e,(hl)
    push de
    ld de,FILE_BODY_IO.header
    call fbField
    ld c,(hl)
    ld b,0
    inc hl
    ld l,(hl)
    ld h,0
    add hl,bc
    ex de,hl
    pop hl
    call sub_hl_de
    jp c,fbBad
    ld b,h
    ld c,l
    ld de,FILE_BODY_IO.buffer
    call fbField
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    call fbRange
    ret c
    push hl
    push bc
    ld hl,fbHandle
    ld b,FILE_HANDLE
    xor a
fbClearHandle:
    ld (hl),a
    inc hl
    dec b
    jp nz,fbClearHandle
    ld hl,(fbRequest)
    ld de,fbHandle
    ld b,FILE_DESCRIPTOR
fbCopyDescriptor:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,fbCopyDescriptor
    ld de,0
    MXOS_CALL_REGS MXOS_DOS_FILE_GET_SET_DRIVE
    ld (fbHandle+FH_DRIVE),a
    ld a,FILE_MODE_READ
    ld (fbHandle+FH_MODE),a
    ld de,FILE_BODY_IO.parent
    call fbField
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld (fbHandle+FH_SEEK),hl
    ld a,FILE_OP_OPEN_AT
    call fbCall
    jp c,fbOpenError
    ld de,FILE_BODY_IO.header
    call fbField
    ld l,(hl)
    ld h,0
    ld (fbHandle+FH_SEEK),hl
    ld a,FILE_OP_SEEK
    call fbCall
    jp c,fbSeekError
    pop bc
    pop hl
    call fbRead
    push af
    push bc
    ld de,FILE_BODY_IO.actual
    call fbField
    pop bc
    ld (hl),c
    inc hl
    ld (hl),b
    pop af
    jp c,fbClose
    ld de,FILE_BODY_IO.tail
    call fbField
    ld c,(hl)
    ld b,0
    ld a,c
    or a
    jp z,fbClose
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    ld a,d
    or e
    jp z,fbClose
    ex de,hl
    call fbRange
    jp c,fbClose
    call fbRead
fbClose:
    push af
    ld a,FILE_OP_CLOSE
    call fbCall
    pop bc
    push af
    pop de
    ld a,c
    and 1
    jp nz,fbFirstError
    push de
    pop af
    ret
fbFirstError:
    push bc
    pop af
    ret
fbSeekError:
    call fbClose
fbOpenError:
    pop bc
    pop hl
    ret
fbRead:
    ld (fbHandle+FH_BUFFER),hl
    ld h,b
    ld l,c
    ld (fbHandle+FH_COUNT),hl
    ld (fbReadCount),hl
    ld a,FILE_OP_READ
    call fbCall
    push af
    ld hl,(fbHandle+FH_COUNT)
    ld b,h
    ld c,l
    ld hl,(fbHandle+FH_BUFFER)
    add hl,bc
    pop af
    ret c
    push hl
    ld hl,(fbReadCount)
    ld a,h
    cp b
    jp nz,fbShortRead
    ld a,l
    cp c
    jp nz,fbShortRead
    pop hl
    xor a
    ret
fbShortRead:
    pop hl
    ld a,DOS_ERR_BAD_IMAGE
    scf
    ret
fbCall:
    ld hl,fbHandle
    MXOS_CALL_REGS MXOS_DOS_FILEHANDLE
    ret
fbHandle    = BIOS_TEMP_BUFFER+128
fbReadCount = io_vars.fbCursor  ; Two bytes; remaining two bytes are reserved.

; HL=приёмник, BC=число; нельзя затереть BIOS и параметры загрузки.
; AF/DE разрушены; HL/BC сохранены. Собственный стек защищает вызывающий.
fbRange:
    push hl
    add hl,bc
    jp c,fbRangeBad
    ld (fbEnd),hl
    ld de,0C000h
    call cmp_hl_de
    jp c,fbRangeBelow
    jp z,fbRangeBelow
    pop hl
    push hl
    ld a,h
    cp SYSTEM_PROGRAM_ADDR/256
    jp c,fbRangeBad
    ld hl,(fbEnd)
    ld de,SYSTEM_PROGRAM_END
    call cmp_hl_de
    jp c,fbRangeBelow
    jp nz,fbRangeBad
fbRangeBelow:
    pop hl
    push hl
    push hl
    ld hl,(fbRequest)
    ex de,hl
    pop hl
    push hl
    ld hl,(fbEnd)
    call cmp_hl_de
    pop hl
    jp c,fbRangeGood
    jp z,fbRangeGood
    push hl
    ld hl,FILE_BODY_IO
    add hl,de
    ex de,hl
    pop hl
    call cmp_hl_de
    jp c,fbRangeBad
fbRangeGood:
    pop hl
    xor a
    ret
fbRangeBad:
    pop hl
fbBad:
    ld a,DOS_ERR_ARGUMENT
    scf
    ret

;----------------------------------------------------------------------------
; MXOS
;
; Получить адрес поля текущего запроса файлового ввода-вывода.
;
; вход:
;   de = смещение внутри структуры, fbRequest содержит адрес её начала.
;
; выход:
;   hl = адрес поля. de, bc и a сохранены; cf определяется 16-битным сложением.
;----------------------------------------------------------------------------

fbField:
    ld hl,(fbRequest)
    add hl,de
    ret
fbRequest = io_vars.fbRequest
fbEnd     = io_vars.fbEnd

fileBodyEnd:
