; Stable LSD over 11 normalized filename bytes and rank. FILE_INFO is immutable.
; Index buffers are page aligned. No comparator or per-item address multiply.
ncRadixPrepare:
    ld hl,(ncSortRecords)
    ld e,0
    ld a,(ncSortCount)
    or a
    ret z
    ld b,a
ncRadixPrepareRow:
    push bc
    push hl
    call ncFileRank
    ld d,NC_SORT_RANKS/256
    ld (de),a
    ld d,NC_SORT_KEYS/256
    ld b,11
ncRadixPrepareByte:
    ld a,(hl)
    call bios_upperCase
    ld (de),a
    inc hl
    inc d
    dec b
    jp nz,ncRadixPrepareByte
    pop hl
    ld bc,16
    add hl,bc
    pop bc
    inc e
    dec b
    jp nz,ncRadixPrepareRow
    ret

ncRadixSort:
    ld a,(ncSortCount)
    cp 2
    ret c
    ld hl,(ncSortBase)
    ld a,h
    ld (ncRadixInput),a
    ld a,NC_SORT_SCRATCH/256
    ld (ncRadixOutput),a
    ld hl,ncRadixNameOrder
    ld a,(ncSortMode)
    cp 2
    jp nz,ncRadixOrderReady
    ld hl,ncRadixExtOrder
ncRadixOrderReady:
    ld (ncRadixOrder),hl
ncRadixPass:
    ld hl,(ncRadixOrder)
    ld a,(hl)
    cp 0FFh
    jp z,ncRadixFinish
    inc hl
    ld (ncRadixOrder),hl
    add a,NC_SORT_KEYS/256
    ld (ncRadixPlane+1),a
    ld h,a
    ld l,0
    ld a,(ncSortCount)
    ld b,a
    ld d,0FFh
    ld e,0
ncRadixRange:
    ld a,(hl)
    cp d
    jp nc,ncRadixNoMin
    ld d,a
ncRadixNoMin:
    cp e
    jp c,ncRadixNoMax
    ld e,a
ncRadixNoMax:
    inc l
    dec b
    jp nz,ncRadixRange
    ld a,d
    cp e
    jp z,ncRadixPass             ; Constant digit cannot change the order.
    ld (ncRadixMinimum),a
    ld l,a
    ld a,e
    sub d
    inc a
    ld (ncRadixSpan),a           ; Zero means all 256 bucket values.
    ld b,a
    ld h,NC_SORT_COUNTS/256
    xor a
ncRadixZero:
    ld (hl),a
    inc l
    dec b
    jp nz,ncRadixZero
    ld a,(ncRadixPlane+1)
    ld h,a
    ld l,0
    ld d,NC_SORT_COUNTS/256
    ld a,(ncSortCount)
    ld b,a
ncRadixHistogram:
    ld e,(hl)                   ; Histogram does not depend on index order.
    ld a,(de)
    inc a
    ld (de),a
    inc l
    dec b
    jp nz,ncRadixHistogram
    ld a,(ncRadixMinimum)
    ld l,a
    ld h,NC_SORT_COUNTS/256
    ld a,(ncRadixSpan)
    ld b,a
    ld c,0
ncRadixPrefix:
    ld a,(hl)
    ld (hl),c
    add a,c
    ld c,a
    inc l
    dec b
    jp nz,ncRadixPrefix
    ld a,(ncRadixOutput)
    ld (ncRadixDestination+1),a
    ld a,(ncRadixInput)
    ld h,a
    ld l,0
    ld a,(ncSortCount)
    ld b,a
ncRadixScatter:
    ld c,(hl)
    push hl
    ld l,c
ncRadixPlane:
    ld h,0
    ld e,(hl)
    ld d,NC_SORT_COUNTS/256
    ld a,(de)
    ld l,a
    inc a
    ld (de),a
ncRadixDestination:
    ld h,0
    ld (hl),c
    pop hl
    inc l
    dec b
    jp nz,ncRadixScatter
    ld a,(ncRadixInput)
    ld b,a
    ld a,(ncRadixOutput)
    ld (ncRadixInput),a
    ld a,b
    ld (ncRadixOutput),a
    jp ncRadixPass
ncRadixFinish:
    ld hl,(ncSortBase)
    ld a,(ncRadixInput)
    cp h
    ret z
    ex de,hl
    ld h,a
    ld l,0
    ld a,(ncSortCount)
    ld b,a
ncRadixFinalCopy:
    ld a,(hl)
    ld (de),a
    inc l
    inc de
    dec b
    jp nz,ncRadixFinalCopy
    ret

ncRadixNameOrder: DB 10,9,8,7,6,5,4,3,2,1,0,11,0FFh
ncRadixExtOrder: DB 7,6,5,4,3,2,1,0,10,9,8,11,0FFh
ncRadixOrder: DW 0
ncRadixInput: DB 0
ncRadixOutput: DB 0
ncRadixMinimum: DB 0
ncRadixSpan: DB 0
