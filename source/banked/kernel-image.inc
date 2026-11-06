; Загрузка образа в страницу клиента без исполнения в странице DOS.
; IMAGEOPEN: HL=32-байтный дескриптор; выход HL=вход, DE=адрес, BC=тело.
; FILELOAD: DE=приёмник тела открытого образа; выход HL=его точка входа.
; FILELOAD2: HL=дескриптор, DE=приёмник всего файла; BC/DE/HL сохранены.
; Все три вызова возвращают CF/A ошибки; страницы/стек сохраняет транспорт.
kiFetchDescriptor:
    ld hl,(kdRegs)
    ld (kdClientObject),hl
    ld de,KD_OBJECT
    ld bc,FILE_DESCRIPTOR
    jp kdFetch
kiOpen:
    call kiFetchDescriptor
    jp c,kdBad
    call dosClearError
    ld hl,KD_OBJECT
    call imageOpen
    call c,dosGetError
    call kdCapture
    ld a,(kdRegs+6)
    and 1
    jp nz,kiReturnDescriptor
    ld a,(kdPage)
    ld (kiOwner),a
    ld a,1
    ld (kiReady),a
kiReturnDescriptor:
    ld hl,(kdClientObject)
    ex de,hl
    ld hl,KD_OBJECT
    ld bc,FILE_DESCRIPTOR
    call kdStore
    jp c,kdBad
    jp kdFinish

kiLoad:
    ld a,(kiReady)
    or a
    jp z,kdBad
    ld a,(kdPage)
    ld hl,kiOwner
    cp (hl)
    jp nz,kdBad
    ld hl,(v_tmpFileDescr+10)
    ld b,h
    ld c,l
    ld hl,(kdRegs+2)
    call kiRange
    jp c,kdBad
    ld (kdFarCurrent),hl
    xor a
    ld (kiReady),a
    ld hl,0
    ld (kiSum),hl
    ld a,1
    ld (kiSumActive),a
    ld (kdFarActive),a
    call dosClearError
    ld de,KD_DATA
    call imageLoad
    call c,dosGetError
    call kdCapture
    xor a
    ld (kiSumActive),a
    ld (kdFarActive),a
    jp kdFinish

; Тело файла не может перекрыть публичные входы F800h. Системная строка
; FA00h допускается для аргументов запросов, но не для загрузки образа.
; HL/BC сохранены; AF/DE разрушены, CF=ошибка диапазона.
kiRange:
    call kdRange
    ret c
    push hl
    add hl,bc
    ld de,mx_call
    call cmp_hl_de
    pop hl
    jp c,kiRangeGood
    jp z,kiRangeGood
    scf
    ret
kiRangeGood:
    or a
    ret

; Контрольная сумма Monitor-2 по частям с прежней семантикой последнего байта.
; BC=длина текущей прочитанной пачки; BC сохранён, AF/DE/HL разрушены.
; Используется штатная петля calcCS_loop с накопленным BC, без нового цикла.
kiSumChunk:
    ld a,b
    or c
    ret z
    push bc
    ld hl,KD_DATA-1
    add hl,bc
    ex de,hl
    ld hl,(kiSum)
    ld b,h
    ld c,l
    ld hl,KD_DATA
    call calcCS_loop
    ; calcCS не добавляет последний байт в старшую часть суммы.
    ; Для промежуточной пачки добавляем его с переносом из последнего ADD.
    push af
    push hl
    ld hl,(ksActual)
    ex de,hl
    ld hl,(ksLeft)
    call cmp_hl_de
    pop hl
    jp z,kiSumLast
    pop af
    ld a,b
    adc a,(hl)
    ld b,a
    jp kiSumSave
kiSumLast:
    pop af
kiSumSave:
    ld h,b
    ld l,c
    ld (kiSum),hl
    pop bc
    ret
kiLoadSum:
    ld hl,(kiSum)
    ld b,h
    ld c,l
    jp imageCheckSum

kiRaw:
    call kiFetchDescriptor
    jp c,kdBad
    call dosClearError
    ld hl,KD_OBJECT
    call fileLoadInfo
    jp c,kiRawReturn
    ld hl,(KD_OBJECT+30)
    ld a,h
    or l
    jp nz,kiRawBad
    ld hl,(KD_OBJECT+28)
    ld b,h
    ld c,l
    ld hl,(kdRegs+2)
    ld (kdFarCurrent),hl
    call kiRange
    jp c,kiRawBad
    push bc
    ld hl,(kdClientObject)
    ex de,hl
    ld hl,(kdFarCurrent)
    ld a,FILE_DESCRIPTOR
    call kdOverlap
    pop bc
    jp c,kiRawBad
    ld a,1
    ld (kdFarActive),a
    ld hl,KD_OBJECT
    ld de,KD_DATA
    call fatFileLoadFound
    push af
    xor a
    ld (kdFarActive),a
    pop af
kiRawReturn:
    call c,dosGetError
    push af
    pop hl
    ld (kdRegs+6),hl
    jp kiReturnDescriptor
kiRawBad:
    call blockArgument
    jp kiRawReturn
kiReady: DB 0
kiOwner: DB 0
kiSumActive: DB 0
kiSum: DW 0
