;----------------------------------------------------------------------------
; MXOS
;
; Верхняя граница приложения RAMFOS. Изменение ограничено нижней MAIN.
;
; вход:
;   ramfosSetTop принимает HL, ramfosGetTop вызывается без параметров.
;
; выход:
;   ramfosGetTop возвращает HL. Остальные регистры и флаги сохранены.
;   ramfosSetTop сохраняет все регистры и флаги.
;----------------------------------------------------------------------------

ramfosGetTop:
    push af
    push de
    ld hl,(ramfos_memTop)
    ex de,hl
    ld hl,(v_memTop)
    call cmp_hl_de
    jp c,.safe
    ex de,hl
.safe:
    pop de
    pop af
    ret
ramfosSetTop:
    push af
    push hl
    ld a,h
    cp high APP_MEMORY_END
    jp c,.store
    ld hl,APP_MEMORY_END-1
.store:
    ld (ramfos_memTop),hl
    pop hl
    pop af
    ret

;----------------------------------------------------------------------------
; MXOS
;
; Чтение и запись одного байта по историческим F836h/F839h.
;
; вход:
;   A=0, HL = адрес нижней MAIN вплоть до фактической границы RAMFOS.
;   ramfosWriteAny принимает C=байт. Страницы ОС закрыты.
;
; выход:
;   CF=0, A=0. ramfosReadAny возвращает C=байт.
;   При отказе CF=1, A=1, для чтения C=FFh. B/DE/HL сохранены.
;----------------------------------------------------------------------------

ramfosReadAny:
    call ramfosCheckMemory
    jp c,ramfosReadFailure
    ld c,(hl)
    xor a
    ret
ramfosWriteAny:
    call ramfosCheckMemory
    jp c,ramfosNoFile
    ld (hl),c
    xor a
    ret
ramfosCheckMemory:
    or a
    scf
    ret nz
    push de
    push hl
    ex de,hl
    call ramfosGetTop
    call cmp_hl_de
    pop hl
    pop de
    ret

ramfos_memTop: DW APP_MEMORY_END-1
