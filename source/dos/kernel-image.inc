; Тело файла не может перекрыть публичные входы F800h. Системная строка
; Системная строка main допускается для аргументов, но не для загрузки образа.
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

kiSumActive: DB 0
kiSum: DW 0
