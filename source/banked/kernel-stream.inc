; Адаптер внутреннего fatStream для буфера чужой страницы.
; Вход как fatStream, исходный AF лежит на стеке его короткого шлюза.
; kdFarCurrent — NEAR клиента; kdFarActive включает маршрут только внутри
; проверенного внешнего запроса. Цепочку, кэш и ошибки обслуживает общий код.
kdFarStream:
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
