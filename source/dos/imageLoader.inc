; COM/EXE: A=16, HL=дескриптор; проверяет первый сектор/заголовок.
; Выход HL=вход, DE=загрузка, BC=тело; CF=ошибка. Тело ещё не прочитано.
; RKS: A=4; выход HL=IMAGE_LOAD_CONTEXT, DE=буфер первого сектора.
; Вызывающий заполняет loadAddress/entryAddress/bodySize после разбора RKS.
; imageLoad (C848h): DE=назначение, читает тело по месту. COM проверяет
; сумму и возвращает HL=вход; RKS возвращает BC=сумма, HL=курсор хвоста,
; DE=следующий кластер — сумму из файла проверяет RUN. CF=ошибка.
; Краевые секторы в штатном кэше, экран и нижнее ОЗУ не служат буферами.
; Контекст v_tmpFileDescr действителен до следующей файловой операции.
imageOpen:
    push af
    call fileLoadInfo
    pop bc
    ret c
    ld a,b
    ld (v_tmpFileDescr+16),a
    ld hl,(v_tmpFileDescr+30)
    ld a,h
    or l
    jp nz,imageError
    ld a,(v_tmpFileDescr+16)
    cp 4
    ld de,17
    jp nz,imageMinimum
    ld de,6
imageMinimum:
    ld hl,(v_tmpFileDescr+28)
    call cmp_hl_de
    jp c,imageError
    call imageCache
    ret c
    ld (v_tmpFileDescr+18),hl
    ld a,(v_tmpFileDescr+16)
    cp 4
    jp z,imageRawHeader
    ld de,v_tmpFileDescr
    ld bc,16
    call moveBytesForward
    ld hl,v_tmpFileDescr
    ld de,imageMagic
    ld b,6
imageMagicLoop:
    ld a,(de)
    cp (hl)
    jp nz,imageError
    inc hl
    inc de
    dec b
    jp nz,imageMagicLoop
    ld hl,v_tmpFileDescr
    ld de,v_tmpFileDescr+13
    call calcCS
    ld hl,(v_tmpFileDescr+14)
    ld d,b
    ld e,c
    call cmp_hl_de
    jp nz,imageError
    ld hl,(v_tmpFileDescr+10)
    ld de,16
    jp imageLength
imageRawHeader:
    ex de,hl
    ld hl,v_tmpFileDescr
    or a
    ret
imageLength:
    ld a,h
    or l
    jp z,imageError
    add hl,de
    jp c,imageError
    push hl
    ld hl,(v_tmpFileDescr+28)
    ex de,hl
    pop hl
    call cmp_hl_de
    jp nz,imageError
    ld hl,(v_tmpFileDescr+6)
    push hl
    ld hl,(v_tmpFileDescr+10)
    ex de,hl
    pop hl
    add hl,de
    jp c,imageError
    push hl
    ld hl,(v_tmpFileDescr+8)
    ex de,hl
    pop hl
    call cmp_hl_de
    jp c,imageError
    jp z,imageError
    ld hl,(v_tmpFileDescr+6)
    ex de,hl
    call cmp_hl_de
    ret c
    ld hl,(v_tmpFileDescr+10)
    ld b,h
    ld c,l
    ld hl,(v_tmpFileDescr+8)
    or a
    ret
imageError:
    scf
    ret

; Следующий сектор через общий кэш; FAT читается до сектора данных,
; чтобы его буфер оставался действителен даже с одной строкой кэша.
; BC сохраняется, HL=буфер, +26 продвинут к следующему кластеру.
imageCache:
    push hl
    ld hl,(v_tmpFileDescr+26)
    ex de,hl
    pop hl
    push de
    call fatReadCluster
    ex de,hl
    ld (v_tmpFileDescr+26),hl
    pop de
    call fatClusterToSector
    xor a
    jp getSectorAddressInCache

imageLoad:
    ex de,hl
    ld (v_input_end),hl
    ex de,hl
    ld a,(v_tmpFileDescr+16)
    cpl
    inc a
    ld e,a
    ld d,0
    ld hl,(v_tmpFileDescr+10)
    call sub_hl_de
    jp nc,imageFirstFull
    add hl,de
    ld b,h
    ld c,l
    ld hl,0
    jp imageFirst
imageFirstFull:
    ld b,d
    ld c,e
imageFirst:
    push hl
    ld hl,(v_tmpFileDescr+18)
    ld a,(v_tmpFileDescr+16)
    ld e,a
    ld d,0
    add hl,de
    push hl
    ld hl,(v_input_end)
    ex de,hl
    pop hl
    call moveBytesForward
    ld (v_tmpFileDescr+18),hl
    ld h,b
    ld l,c
    pop bc
    ld a,b
    or a
    jp z,imageTail
    push bc
    ld c,b
    ld b,2
    push hl
    ld hl,(v_tmpFileDescr+26)
    ex de,hl
    pop hl
    call fileTransferClusters
    ex de,hl
    ld (v_tmpFileDescr+26),hl
    ex de,hl
    pop bc
    ret c
imageTail:
    ld a,c
    or a
    jp z,imageChecksum
    push hl
    call imageCache
    pop de
    ret c
    ld b,0
    call moveBytesForward
    ld (v_tmpFileDescr+18),hl
    ld h,b
    ld l,c
imageChecksum:
    dec hl
    ex de,hl
    ld hl,(v_input_end)
    call calcCS
    ld a,(v_tmpFileDescr+16)
    cp 4
    jp z,imageRksChecksum
    push hl
    ld hl,(v_tmpFileDescr+12)
    ex de,hl
    pop hl
    jp imageCompareChecksum
imageRksChecksum:
    ld hl,(v_tmpFileDescr+26)
    ex de,hl
    ld hl,(v_tmpFileDescr+18)
    or a
    ret
imageCompareChecksum:
    ld h,b
    ld l,c
    call cmp_hl_de
    jp nz,imageError
    ld hl,(v_tmpFileDescr+8)
    or a
    ret
imageMagic: DB 0CDh,020h,"MX",1,0
