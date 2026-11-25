;----------------------------------------------------------------------------
; MXOS
; Найти выровненный интервал сверху вниз в DOS или MAIN.
; вход: BC = интервал с minHeap, HL = маска выравнивания, требования в v_imageHeader.
; выход: HL = база, CF=0, либо CF=1/A=MEM_ERROR_NOMEM. AF/BC/DE изменены.
; Проба не вытесняет образы. Второй проход разрешает вытеснение при окончательном занятии участка.
;----------------------------------------------------------------------------
relocDosHole:
    xor a
    jp rhStart
relocMainHole:
    ld a,1
rhStart:
    ld (relocHoleMain),a
    ld (relocHoleMask),hl
    MD_PUT_BC relocHoleSpan
    xor a
    ld (relocHolePass),a
rhRestart:
    ld hl,DOS_BANK_DRIVER_END
    ld a,(relocHoleMain)
    or a
    jp z,rhCandidate
    ld hl,(memorySystemEnd)
rhCandidate:
    MD_GET_DE relocHoleSpan
    call sub_hl_de
    jp c,rhNextPass
    MD_GET_DE relocHoleMask
    ld a,e
    cpl
    and l
    ld l,a
    ld a,d
    cpl
    and h
    ld h,a
    ld a,(relocHoleMain)
    or a
    jp nz,rhTry
    ld de,DOS_ARENA_START
    call cmp_hl_de
    jp c,rhNextPass
rhTry:
    push hl
    MD_GET_BC relocHoleSpan
    add hl,bc
    ex de,hl
    pop hl
    ld a,(relocHoleMain)
    or a
    jp z,rhTryDos
    call rhCheckMain
    jp rhChecked
rhTryDos:
    ld a,(relocHolePass)
    or a
    jp nz,rhPolicyProbe
    call arenaProbe
    jp c,rhBlocked
    push hl
    ld hl,(batWindowEnd)
    ld a,h
    or l
    jp z,rhNoBat
    ex de,hl
    ld hl,(BAT_HANDLE+FH_BUFFER)
    call arenaOverlap
rhNoBat:
    pop hl
    ret z
    jp rhBlocked
rhPolicyProbe:
    ld a,(v_imageHeader+IMAGE_POLICY)
    ld (arenaPolicy),a
    ld a,3
    call arenaEnter
rhChecked:
    ret nc
rhBlocked:
    ld hl,(arenaObstacle)
    jp rhCandidate
rhNextPass:
    ld a,(relocHolePass)
    or a
    jp nz,rhNoMemory
    inc a
    ld (relocHolePass),a
    jp rhRestart
rhNoMemory:
    ld a,MEM_ERROR_NOMEM
    scf
    ret

;----------------------------------------------------------------------------
; MXOS
; Проверить прежнюю базу vacant-записи, не изменяя таблиц или памяти DLL.
; вход: DE=база, BC=размер с minHeap, HL=маска.
; выход: CF=0/HL=база либо CF=1, AF/BC/DE рабочие.
;----------------------------------------------------------------------------
relocMainProbeReuse:
    ld a,d
    and h
    ld h,a
    ld a,e
    and l
    or h
    scf
    ret nz
    ex de,hl
    push hl
    add hl,bc
    ex de,hl
    pop hl
    ret c
    push hl
    ld hl,(memorySystemEnd)
    call cmp_hl_de
    pop hl
    ret c
    xor a
    ld (relocHolePass),a
rhCheckMain:
    push bc
    push de
    push hl
    ld (mmcStart),hl
    ex de,hl
    ld (mmcEnd),hl
    ld a,2
    ld (mmcPhase),a
    xor a
    ld (mmcAllowOwner),a
    ld a,(relocHolePass)
    or a
    ld a,IMAGE_POLICY_PRESERVE
    jp z,rhHavePolicy
    ld a,(v_imageHeader+IMAGE_POLICY)
rhHavePolicy:
    ld (mmcPolicy),a
    ld a,(appDepth)
    or a
    jp z,mmcRestart
    ld b,a
    ld hl,appHeapBounds
rhParent:
    push bc
    ld c,(hl)
    inc hl
    ld b,(hl)
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    push hl
    ld h,b
    ld l,c
    call cmp_hl_de
    jp c,rhSkipParent
    jp z,rhSkipParent
    call mmcOverlap
    jp nc,rhSkipParent
    pop hl
    pop bc
    scf
    jp mmcDone
rhSkipParent:
    pop hl
    pop bc
    dec b
    jp nz,rhParent
    jp mmcRestart

;----------------------------------------------------------------------------
; MXOS
; Найти две границы минимальной кучи родителя.
; вход: A = индекс приложения 0..APP_MAX_DEPTH-1.
; выход: HL = адрес слов конец/начало, DE и флаги рабочие, BC сохранён.
;----------------------------------------------------------------------------
appHeapAddress:
    add a,a
    add a,a
    ld e,a
    ld d,0
    ld hl,appHeapBounds
    add hl,de
    ret
