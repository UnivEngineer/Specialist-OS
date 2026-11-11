; GET32/PUT32: A=FAR-банк, HL=адрес, BC:DE=значение (DE младшее).
; GET сохраняет HL; PUT сохраняет BC/DE/HL. CF/A=статус.
; Шаблон атомарно исполняется в общем окне, стек чужого банка не используется.
; COPY-профиль остаётся активным; следующий memcpy заново задаёт параметры,
; а COMMAND восстанавливается из сохранённого снимка обычным транспортом.
memGetDword:
    push bc
    push de
    push hl
    push af
    ld hl,memGetDwordTemplate
    jp memDword
memPutDword:
    push bc
    push de
    push hl
    push af
    ld hl,memPutDwordTemplate
    ld a,(bankPage)
    or a
    jp nz,memDword
    ld hl,MEM_MAIN_PUT32_TEMPLATE
memDword:
    ld a,BANK_PROFILE_COPY
    call memProfile
    ld de,BANK_WINDOW
    ld b,27
    call bankCopyBytes
    ld hl,memDwordReturn
    ld (BANK_WINDOW+25),hl
    pop af
    pop hl
    push hl
    call bankCopyPageCheck
    jp c,memDwordError
    ld (BANK_WINDOW+8),hl
    ld a,h
    cp 0FFh
    jp nc,memDwordRange
    inc hl
    inc hl
    inc hl
    ld a,h
    cp 0FFh
    jp nc,memDwordRange
    ld a,c
    call bankCopyPair
    ld (BANK_WINDOW+1),hl
    ld a,(bankPage)
    call bankCopyPair
    ld (BANK_WINDOW+18),hl
    pop hl
    pop de
    pop bc
    push hl
    jp BANK_WINDOW
memDwordRange:
    ld a,MEM_ERROR_RANGE
    scf
memDwordError:
    pop hl
    pop de
    pop bc
    ret
memDwordReturn:
    pop hl
    xor a
    ret
memGetDwordTemplate:
    ld hl,0
    ld a,h
    ld h,0FFh
    ld (hl),a
    ld hl,0
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld c,(hl)
    inc hl
    ld b,(hl)
    ld hl,0
    ld a,h
    ld h,0FFh
    ld (hl),a
    jp memDwordReturn
memGetDwordTemplateEnd:
    ASSERT memGetDwordTemplateEnd-memGetDwordTemplate == 27
