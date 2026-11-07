; FileStream: A=1/2, HL=NEAR-буфер клиента, DE=курсор, BC=байты.
; Возвращает HL=следующий байт, DE=исходный курсор, BC=передано, AF=ошибка.
; Проверенный FAR-буфер использует общий fatStream; курсор остаётся локальным.
kfStream:
    ld a,(kdRegs+7)
    dec a
    cp 2
    jp nc,kfBad
    call dosClearError
    ld hl,(kdRegs+4)
    ld a,h
    or l
    jp z,kfEmpty
    ld b,h
    ld c,l
    ld (kfCount),hl
    ld hl,(kdRegs)
    ld (kdFarCurrent),hl
    call kdRange
    jp c,kfBad
    ld hl,(kdRegs+2)
    ld (kfCursor),hl
    push bc
    ld bc,FILE_STREAM_CURSOR
    call kdRange
    pop bc
    jp c,kfBad
    ex de,hl
    ld hl,(kdFarCurrent)
    ld a,FILE_STREAM_CURSOR
    call kdOverlap
    jp c,kfBad
    ld hl,(kfCursor)
    ld de,KD_OBJECT
    ld bc,FILE_STREAM_CURSOR
    call kdFetch
    jp c,kfBad
    call fatCheckMounted
    jp c,kfError
    ld hl,(KD_OBJECT+2)
    ld (v_streamOffset),hl
    ex de,hl
    ld hl,(v_diskInfo.clusterBytes)
    call cmp_hl_de
    jp c,kfBad
    ld hl,(kfCount)
    ld b,h
    ld c,l
    ld hl,(KD_OBJECT)
    ex de,hl
    ld hl,KD_DATA
    ld a,1
    ld (kdFarActive),a
    ld a,(kdRegs+7)
    call fatStream
    call c,dosIoError
    call dosGetError
    push af
    xor a
    ld (kdFarActive),a
    pop af
    call kdCapture
    ld hl,(v_streamRemaining)
    ex de,hl
    ld hl,(kfCount)
    call sub_hl_de
    ld (kdRegs+4),hl
    ld hl,(v_streamCluster)
    ld (KD_OBJECT),hl
    ld hl,(v_streamOffset)
    ld (KD_OBJECT+2),hl
    ld hl,(kfCursor)
    ld (kdRegs+2),hl
    ex de,hl
    ld hl,KD_OBJECT
    ld bc,FILE_STREAM_CURSOR
    call kdStore
    jp c,kfBad
    jp kdFinish
kfBad:
    call blockArgument
kfError:
    ld hl,0
    ld (kdRegs+4),hl
    jp kdError
kfEmpty:
    call dosClearError
    jp kdError
kfCursor: DW 0
kfCount: DW 0
