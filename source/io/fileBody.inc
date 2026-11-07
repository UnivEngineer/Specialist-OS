; Загрузка тела без разбора формата контейнера, только из main.
; HL=FILE_BODY_IO с готовым дескриптором выбранного диска.
; Выход CF/A=ошибка DOS, actual=передано тело; BC/DE/HL/флаги разрушены.
; Заголовок должен помещаться в первый кластер. Хвост необязателен.
; Нереентерабельно: как FILESTREAM; код, стек и запрос нельзя затирать.
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
    ld a,c
    ld (fbCursor+2),a
    xor a
    ld (fbCursor+3),a
    inc hl
    ld l,(hl)
    ld h,0
    add hl,bc
    ex de,hl
    pop hl
    call bios_sub_hl_de
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
    ld de,FILE_DESCRIPTOR.firstCluster
    call fbField
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld (fbCursor),hl
    pop hl
    ld de,fbCursor
    ld a,2
    call bios_fileStream
    push af
    push bc
    ld de,FILE_BODY_IO.actual
    call fbField
    pop bc
    ld (hl),c
    inc hl
    ld (hl),b
    pop af
    ret c
    ld de,FILE_BODY_IO.tail
    call fbField
    ld c,(hl)
    ld b,0
    ld a,c
    or a
    ret z
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    ld a,d
    or e
    ret z
    ex de,hl
    call fbRange
    ret c
    ld de,fbCursor
    ld a,2
    jp bios_fileStream

; HL=приёмник, BC=число; нельзя затереть BIOS и параметры загрузки.
; AF/DE разрушены; HL/BC сохранены. Собственный стек защищает вызывающий.
fbRange:
    push hl
    add hl,bc
    jp c,fbRangeBad
    ld (fbEnd),hl
    ld de,0C000h
    call bios_cmp_hl_de
    jp c,fbRangeBelow
    jp z,fbRangeBelow
    pop hl
    push hl
    ld a,h
    cp SYSTEM_PROGRAM_ADDR/256
    jp c,fbRangeBad
    ld hl,(fbEnd)
    ld de,SYSTEM_PROGRAM_END
    call bios_cmp_hl_de
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
    call bios_cmp_hl_de
    pop hl
    jp c,fbRangeGood
    jp z,fbRangeGood
    push hl
    ld hl,FILE_BODY_IO
    add hl,de
    ex de,hl
    pop hl
    call bios_cmp_hl_de
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
fbField:
    ld hl,(fbRequest)
    add hl,de
    ret
fbRequest = io_vars.fbRequest
fbEnd = io_vars.fbEnd
fbCursor = io_vars.fbCursor
fileBodyEnd:
