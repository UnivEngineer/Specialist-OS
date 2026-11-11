; Родной контейнер COM/EXE MXOS: HL=имя/дескриптор.
; imageOpen: HL=вход, DE=загрузка, BC=тело, CF=ошибка.
; imageLoad: DE=приёмник, HL=вход после чтения и проверки суммы.
; Контекст внутренний и действителен до следующей файловой операции.
imageOpen:
    call fileLoadInfo
    ret c
    ld hl,(v_tmpFileDescr+30)
    ld a,h
    or l
    jp nz,imageError
    ld de,17
    ld hl,(v_tmpFileDescr+28)
    call cmp_hl_de
    jp c,imageError
    call imageReadPrefix
    ret c
    ld de,v_tmpFileDescr
    ld bc,16
    call moveBytesForward
    ld hl,v_tmpFileDescr
    ld de,imageMagic
    ld b,5
imageMagicLoop:
    ld a,(de)
    cp (hl)
    jp nz,imageError
    inc hl
    inc de
    dec b
    jp nz,imageMagicLoop
    ld a,(hl)
    cp IMAGE_FLAG_MAIN
    jp z,imageFlagsReady
    cp IMAGE_FLAG_BANK+1
    jp nc,imageError
imageFlagsReady:
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
    IFDEF DOS_SPLIT_BUILD
    ld a,DOS_ERR_BAD_IMAGE
    jp dosSetError
    ELSE
    scf
    ret
    ENDIF

; Заголовок читается общим потоком; курсор сохраняется отдельно от кэша.
imageReadPrefix:
    ld hl,0
    ld (v_streamOffset),hl
    ld hl,(v_tmpFileDescr+26)
    ex de,hl
    ld bc,16
    ld hl,v_imageHeader
    ld a,2
    call fatStream
    ret c
    call imageSaveCursor
    ld hl,v_imageHeader
    ret
imageSaveCursor:
    ex de,hl
    ld (v_tmpFileDescr+26),hl
    ld hl,(v_streamOffset)
    ld (v_tmpFileDescr+20),hl
    ret
imageLoad:
    ex de,hl
    ld (v_input_end),hl
    push hl
    ld hl,(v_tmpFileDescr+20)
    ld (v_streamOffset),hl
    ld hl,(v_tmpFileDescr+10)
    ld b,h
    ld c,l
    ld hl,(v_tmpFileDescr+26)
    ex de,hl
    pop hl
    ld a,2
    call fatStream
    ret c
    call fatStreamFinish
    ret c
    push hl
    call imageSaveCursor
    pop hl
    IFDEF BANKED_DOS_BUILD
    ld a,(kdFarActive)
    or a
    jp nz,kiLoadSum
    ENDIF
    dec hl
    ex de,hl
    ld hl,(v_input_end)
    call calcCS
imageCheckSum:
    ld hl,(v_tmpFileDescr+12)
    ld d,b
    ld e,c
    call cmp_hl_de
    jp nz,imageError
    ld hl,(v_tmpFileDescr+8)
    or a
    ret
; v_imageHeader использует временный хвост dosModuleName (dosModule.inc).
imageMagic: DB 0CDh,020h,"MX",1,0
