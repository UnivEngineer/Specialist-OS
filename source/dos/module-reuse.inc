; Vacant MAIN DLLs have handle=0 and active=1. Their last registered
; image interval stays in MODULE_TABLE. The parallel keys occupy 8 bytes
; per slot: FAT cluster, drive and volume epoch (7 bytes plus stride padding).
mdReuseSelect:
    xor a
    ld (mdReuseHit),a
    ld a,(v_imageHeader+IMAGE_OPTIONS)
    and IMAGE_OPTION_REUSABLE
    call nz,mdReuseBuildKey
    ld hl,1 ; Tagged null before the first available slot.
    ld (mdReuseSpare),hl
    ld hl,MODULE_TABLE+(MODULE_MAX-1)*MODULE_RECORD_SIZE
    ld b,MODULE_MAX
mdReuseScan:
    ld a,(hl)
    inc hl
    or (hl)
    dec hl
    jp nz,mdReuseAdvance
    inc hl
    inc hl
    ld a,(hl)
    or a
    jp nz,mdReuseVacant
    dec hl
    dec hl
    ld (mdReuseSpare),hl
    jp mdReuseAdvance
mdReuseVacant:
    dec hl
    dec hl
    push bc
    push hl
    ld a,(v_imageHeader+IMAGE_OPTIONS)
    and IMAGE_OPTION_REUSABLE
    call nz,mdReuseCompareKey
    pop hl
    pop bc
    cp 1
    jp z,mdReuseMatched
    ld a,(mdReuseSpare)
    rrca
    jp nc,mdReuseAdvance
    ; Bit 0 marks a vacant fallback. An empty slot always takes precedence.
    inc hl
    ld (mdReuseSpare),hl
    dec hl
mdReuseAdvance:
    ld de,-MODULE_RECORD_SIZE
    add hl,de
    dec b
    jp nz,mdReuseScan
    ld hl,(mdReuseSpare)
    ld a,h
    or a
    scf
    ret z
    ld a,l
    and 0FEh
    ld l,a
mdReuseOwner:
    ld a,l
    sub MODULE_TABLE & 0FFh
    rrca
    rrca
    rrca
    rrca
    add a,MODULE_OWNER_BASE
    ld c,a
    or a
    ret
mdReuseMatched:
    ld (mdReuseHit),a
    jp mdReuseOwner

mdReuseBuildKey:
    ld hl,(v_tmpFileDescr+26)
    ld (mdReuseKey),hl
    ld a,(v_drive)
    ld (mdReuseKey+2),a
    add a,a
    add a,a
    ld l,a
    ld h,0
    ld de,cliDriveEpochs
    add hl,de
    ld de,mdReuseKey+3
    ld bc,4
    jp moveBytesForward

mdReuseCompareKey:
    call mdReuseKeyAddress
    ld de,mdReuseKey
    ld b,7
mdReuseCompare:
    ld a,(de)
    cp (hl)
    jp nz,mdReuseMiss
    inc de
    inc hl
    dec b
    jp nz,mdReuseCompare
    ld a,1
    ret
mdReuseMiss:
    xor a
    ret
mdReuseTrack:
    ld a,(v_imageHeader+IMAGE_OPTIONS)
    and IMAGE_OPTION_REUSABLE
    ret z
    ld hl,(mdRecord)
    call mdReuseKeyAddress
    ex de,hl
    ld hl,mdReuseKey
    ld bc,7
    jp moveBytesForward
mdReuseKeyAddress:
    ld a,l
    sub MODULE_TABLE & 0FFh
    rrca
    ld l,a
    ld h,0
    ld de,mdReuseKeys
    add hl,de
    ret

mdReuseInvalidateAllocation:
    ld hl,(memorySize)
    ex de,hl
    ld hl,(memoryCandidate)
    add hl,de
    ex de,hl
    ld hl,(memoryCandidate)
; HL=begin, DE=end exclusive. BC/DE/HL preserved.
mdReuseInvalidate:
    push bc
    push de
    push hl
    ld (mdReuseBegin),hl
    ex de,hl
    ld (mdReuseEnd),hl
    ld hl,MODULE_TABLE
    ld b,MODULE_MAX
mdReuseInvalidateNext:
    ld a,(hl)
    inc hl
    or (hl)
    inc hl
    jp nz,mdReuseInvalidateSkip
    ld a,(hl)
    or a
    call nz,mdReuseInvalidateOne
mdReuseInvalidateSkip:
    ld de,MODULE_RECORD_SIZE-2
    add hl,de
    dec b
    jp nz,mdReuseInvalidateNext
    pop hl
    pop de
    pop bc
    ret
mdReuseInvalidateOne:
    push hl
    ld de,8
    add hl,de
    ld a,(hl)
    inc hl
    ld h,(hl)
    ld l,a
    ex de,hl
    ld hl,(mdReuseEnd)
    call cmp_hl_de
    pop hl
    ret c
    ret z
    push hl
    ld de,10
    add hl,de
    ld a,(hl)
    inc hl
    ld h,(hl)
    ld l,a
    ex de,hl
    ld hl,(mdReuseBegin)
    call cmp_hl_de
    pop hl
    ret nc
    ld (hl),0
    ret

; C=reason. Opt-in MAIN DLLs receive scalar arguments and cannot call DOS.
mdActivate:
    ld a,c
    ld (mdReuseRegs+4),a
    ld hl,(mdEntry)
    ld b,h
    ld c,l
    ld de,mdReuseRegs
    ld hl,bankCallback
    xor a
    call bankCallPacket
    ret c
    ld a,(BANK_FRAME_STATUS)
    or a
    ret z
    ld a,MEM_ERROR_RANGE
    scf
    ret

mdReuseHit: DB 0
mdReuseSpare: DW 0
mdReuseBegin: DW 0
mdReuseEnd: DW 0
mdReuseKey: DS 7,0
mdReuseKeys: DS (MODULE_MAX-1)*8+7,0
mdReuseRegs: DB 0,0,0,0,0,0,0,MODULE_ACTIVATE
