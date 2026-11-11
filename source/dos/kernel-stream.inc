; Адаптер внутреннего fatStream для буфера чужой страницы.
; Вход как fatStream, исходный AF лежит на стеке его короткого шлюза.
; kdFarCurrent — NEAR клиента; kdFarActive включает маршрут только внутри
; проверенного внешнего запроса. Цепочку, кэш и ошибки обслуживает общий код.
kdFarStream:
    pop af
    push af
    call ksSelectRoute
    jp nz,ksBuffered
    pop af
    push af
    ld a,1
    ld (ksDirect),a
    ld hl,(kdFarCurrent)
    ld (ksDirectStart),hl
    pop af
    call fatStreamLocal
    ld (kdFarCurrent),hl
    call nc,ksDirectSum
    push af
    xor a
    ld (ksDirect),a
    pop af
    ret
ksBuffered:
    pop af
    ld (ksMode),a
    ex de,hl
    ld (v_streamCluster),hl
    ld h,b
    ld l,c
    ld (ksLeft),hl
ksLoop:
    ld hl,(ksLeft)
    ld a,h
    cp KD_CHUNK/256
    jp c,ksChunkReady
    ld hl,KD_CHUNK
ksChunkReady:
    ld (ksChunk),hl
    ld b,h
    ld c,l
    ld a,(ksMode)
    dec a
    jp nz,ksCall
    ld hl,(kdFarCurrent)
    ld de,KD_DATA
    call kdFetch
    jp c,ksCopyError
ksCall:
    ld hl,(v_streamCluster)
    ex de,hl
    ld hl,KD_DATA
    ld a,(ksMode)
    call fatStreamLocal
    push af
    pop hl
    ld (ksResult),hl
    ld hl,(v_streamRemaining)
    ex de,hl
    ld hl,(ksChunk)
    call sub_hl_de
    ld (ksActual),hl
    ld b,h
    ld c,l
    ld a,(ksMode)
    cp 2
    jp nz,ksAdvance
    ; Сумма образа считается на уже прочитанной пачке, до её пересылки.
    ld a,(kiSumActive)
    or a
    call nz,kiSumChunk
    ld hl,(kdFarCurrent)
    ex de,hl
    ld hl,KD_DATA
    call kdStore
    jp c,ksCopyError
ksAdvance:
    ld hl,(ksActual)
    ex de,hl
    ld hl,(kdFarCurrent)
    add hl,de
    ld (kdFarCurrent),hl
    ld hl,(ksLeft)
    call sub_hl_de
    ld (ksLeft),hl
    ld a,(ksResult)
    and 1
    jp nz,ksReturn
    ld a,h
    or l
    jp nz,ksLoop
ksReturn:
    ld hl,(ksLeft)
    ld (v_streamRemaining),hl
    ld hl,(v_streamCluster)
    ex de,hl
    ld hl,(ksResult)
    push hl
    ld hl,(kdFarCurrent)
    ld (v_streamPtr),hl
    pop af
    ret
ksCopyError:
    call blockArgument
    push af
    pop hl
    ld (ksResult),hl
    jp ksReturn
kdFarActive: DB 0
kdFarCurrent: DW 0
ksMode: DB 0
ksLeft: DW 0
ksChunk: DW 0
ksActual: DW 0
ksResult: DW 0
ksDirect: DB 0
ksDirectStart: DW 0

; Загруженный образ суммируется в странице назначения: назад данные не идут.
; AF=статус, остальные регистры сохранены. BIOS использует прежнюю сумму.
ksDirectSum:
    ld a,(kiSumActive)
    or a
    ret z
    push bc
    push de
    push hl
    dec hl
    ld (klRegs+2),hl
    ld hl,(ksDirectStart)
    ld (klRegs),hl
    ld a,(kdPage)
    or a
    ld bc,bios_calcCS
    jp z,ksSumInvoke
    ld bc,MEM_IO_CHECKSUM
ksSumInvoke:
    ld hl,bankCallback
    ld de,klRegs
    call bankCallPacket
    ld h,b
    ld l,c
    ld (kiSum),hl
    ld a,(BANK_FRAME_STATUS)
    or a
    jp z,ksSumResult
    ld a,DOS_ERR_IO
    scf
ksSumResult:
    pop hl
    pop de
    pop bc
    ret

; Полный сектор идёт непосредственно в клиент. Неполный фрагмент остаётся
; в кэше DOS: копируем только запрошенные байты и сохраняем остальной сектор.
ksSector:
    ld a,(ksDirect)
    or a
    jp z,logicalSectorPrepared
    ; Логический сектор может состоять из нескольких 256-байтных блоков.
    push bc
    push de
    push hl
    ld a,(v_logicalShift)
    ld c,1
    or a
    jp z,ksBlocks
    push hl
    ex de,hl
ksScale:
    add hl,hl
    inc c
    dec a
    jp nz,ksScale
    ex de,hl
    pop hl
ksBlocks:
    call klBlock
    jp c,ksBlocksDone
    inc h
    inc de
    dec c
    jp nz,ksBlocks
ksBlocksDone:
    pop hl
    pop de
    pop bc
    ret
ksCacheCopy:
    ld a,(ksDirect)
    or a
    jp z,moveBytesForward
    ld a,(v_streamMode)
    dec a
    jp z,kdFetch
    jp kdStore

; Маршрут сохраняется на внешний запрос, отдельно для выбранного диска.
ksSelectRoute:
    push hl
    ld a,(v_drive)
    ld hl,ksRouteDrive
    cp (hl)
    jp z,ksRouteKnown
    ld (hl),a
    call klEligible
    ld a,0
    jp z,ksRouteSave
    inc a
ksRouteSave:
    ld (ksRouteBuffered),a
ksRouteKnown:
    ld a,(ksRouteBuffered)
    or a
    pop hl
    ret
ksRouteDrive: DB 0FFh
ksRouteBuffered: DB 1
