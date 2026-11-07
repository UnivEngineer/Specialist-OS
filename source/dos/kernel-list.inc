; Список FILE_INFO строится общим обходчиком каталога за один проход.
; Полные пачки сразу уходят в страницу клиента; общий счётчик/skip/capacity
; остаётся у FAT. Последняя пачка содержит завершающий FFh.
kdFileList:
    ld hl,fileList
    ld (kdEntry),hl
kdList:
    ld hl,(kdRegs)
    ld (kdClientObject),hl
    ld (klCursor),hl
    ; Ёмкость * FILE_INFO_SIZE + терминатор без 16-битного переполнения.
    ld hl,(kdRegs+2)
    ld a,h
    and 0F0h
    jp nz,kdBad
    add hl,hl
    add hl,hl
    add hl,hl
    add hl,hl
    inc hl
    ld b,h
    ld c,l
    ld hl,(kdClientObject)
    call kdRange
    jp c,kdBad
    ld hl,KD_DATA
    ld (kdRegs),hl
    xor a
    ld (klFailed),a
    inc a
    ld (klActive),a
    call kdInvoke
    call kdCapture
    xor a
    ld (klActive),a
    jp kdRestorePointer

; DE=следующий байт локального списка. HL/BC сохраняются, AF разрушен.
; При полной пачке DE возвращается к KD_DATA; CF=ошибка передачи.
kdListStep:
    ld a,(klActive)
    or a
    ret z
    ld a,d
    cp (KD_DATA+KD_CHUNK)/256
    jp nz,klStepDone
    push hl
    push bc
    ld bc,KD_CHUNK
    call klFlush
    pop bc
    pop hl
    ld de,KD_DATA
    ret
klStepDone:
    or a
    ret

; DE=место FFh. Сохраняет HL/BC/DE и исходный AF окончания FAT-обхода,
; кроме ошибки передачи, которая становится итоговым кодом DOS.
kdListEnd:
    push af
    ld a,(klActive)
    or a
    jp z,klEndInactive
    ld a,(klFailed)
    or a
    jp nz,klEndFailed
    push hl
    push bc
    push de
    ld a,0FFh
    ld (de),a
    ex de,hl
    ld de,KD_DATA
    call sub_hl_de
    inc hl
    ld b,h
    ld c,l
    call klFlush
    pop de
    pop bc
    pop hl
    jp c,klEndFailed
klEndInactive:
    pop af
    ret
klEndFailed:
    pop af
    jp blockArgument

; BC=длина пачки. BC сохранён; AF/DE/HL разрушены.
klFlush:
    ld hl,(klCursor)
    ex de,hl
    ld hl,KD_DATA
    call kdStore
    jp c,klCopyFailed
    ld hl,(klCursor)
    add hl,bc
    ld (klCursor),hl
    or a
    ret
klCopyFailed:
    ld a,DOS_ERR_ARGUMENT
    ld (klFailed),a
    jp blockArgument
klActive: DB 0
klFailed: DB 0
klCursor: DW 0
