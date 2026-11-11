; Isolated NC index-sort benchmark. No DOS, drawing or font preparation here.
FILE_INDEX_A = 06000h
FILE_INDEX_B = 06100h
NC_SORT_RADIX = 1
NC_SORT_KEYS = 06300h
NC_SORT_RANKS = NC_SORT_KEYS + 11 * 256
NC_SORT_COUNTS = 06F00h
NC_SORT_SCRATCH = 07000h
benchScratch = 06100h
benchCounts = 06200h
benchRaw = 04000h
benchNormalized = 05000h

benchRun:
    IFNDEF SORTBEN_STANDALONE
    ld sp,08B00h
    ENDIF
    call benchFill
    call benchTimerInit
    call pitTimerRead
    ld (benchStart),hl
benchInvoke:
    call ncSortPass
    call pitTimerRead
    ex de,hl
    ld hl,(benchStart)
    ld a,l
    sub e
    ld l,a
    ld a,h
    sbc a,d
    ld h,a
    ld (benchElapsed),hl
benchDone:
    IFDEF SORTBEN_STANDALONE
    ret
    ELSE
    jp benchDone
    ENDIF

benchPrepareRun:
    IFNDEF SORTBEN_STANDALONE
    ld sp,08B00h
    ENDIF
    call benchTimerInit
    call pitTimerRead
    ld (benchStart),hl
benchPrepareInvoke:
    call benchPrepare
    call pitTimerRead
    ex de,hl
    ld hl,(benchStart)
    ld a,l
    sub e
    ld l,a
    ld a,h
    sbc a,d
    ld h,a
    ld (benchElapsed),hl
    jp benchDone

benchOptimizedPrepare:
    ld a,(state.panelA_filesCnt)
    ld (ncSortCount),a
    call fileCacheBase
    ld (ncSortRecords),hl
    jp ncRadixPrepare

benchPrepare:
    ld hl,benchRaw
    ld de,benchNormalized
    ld a,(state.panelA_filesCnt)
    or a
    ret z
    ld b,a
benchPrepareRow:
    push bc
    push hl
    call ncFileRank
    ld (benchRank),a
    ld c,11
benchPrepareText:
    ld a,(hl)
    call bios_upperCase
    ld (de),a
    inc hl
    inc de
    dec c
    jp nz,benchPrepareText
    ld a,(hl)
    ld (de),a
    inc de
    ld a,(benchRank)
    ld (de),a
    inc de
    xor a
    ld (de),a
    inc de
    ld (de),a
    inc de
    ld (de),a
    inc de
    pop hl
    ld bc,16
    add hl,bc
    pop bc
    dec b
    jp nz,benchPrepareRow
    ret

; Transposed normalized keys: one 256-byte plane per filename byte.
; Record indices address a key byte directly, with no index*16 or strcmp.
benchPreparePlanes:
    ld hl,benchRaw
    ld e,0
    ld a,(state.panelA_filesCnt)
    or a
    ret z
    ld b,a
benchPreparePlaneRow:
    push bc
    push hl
    call ncFileRank
    ld d,benchNormalized/256+12
    ld (de),a
    ld d,benchNormalized/256
    ld c,11
benchPreparePlaneByte:
    ld a,(hl)
    call bios_upperCase
    ld (de),a
    inc hl
    inc d
    dec c
    jp nz,benchPreparePlaneByte
    pop hl
    ld bc,16
    add hl,bc
    pop bc
    inc e
    dec b
    jp nz,benchPreparePlaneRow
    ret

benchPlaneKey:
    ld a,(benchI)
    ld l,a
    ld h,FILE_INDEX_A/256
    ld l,(hl)
    ld a,(benchOffset)
    add a,benchNormalized/256
    ld h,a
    ld a,(hl)
    ret

; Configurable divider: 256 normally, 4096 for quadratic insertion cases.
; The latter gives a 134-second range rather than silently wrapping at 8 s.
benchTimerInit:
    ld hl,IO_TIMER_MODE
    ld (hl),036h
    ld (hl),074h
    dec hl
    dec hl
    xor a
    ld (hl),a
    ld a,(benchDivisorHigh)
    ld (hl),a
    inc hl
    inc hl
    ld (hl),0B4h
    dec hl
    xor a
    ld (hl),a
    ld (hl),a
    ret

benchFill:
    ld hl,FILE_INDEX_A
    ld (ncSortBase),hl
    ld a,(state.panelA_filesCnt)
    ld (ncSortCount),a
    ld b,a
    or a
    ld c,0
    jp z,benchFilled
benchFillLoop:
    ld (hl),c
    inc hl
    inc c
    dec b
    jp nz,benchFillLoop
benchFilled:
    ld (hl),0FFh
    ld a,(ncSortCount)
    or a
    rra
    ld (ncSortGap),a
    ld a,(state.panelA_sort)
    ld (ncSortMode),a
    call ncSortSetup
    call ncPrepareRanks
    ld hl,(ncSortComparator)
    ld (benchQuickCompare+1),hl
    ld (benchMergeCompare+1),hl
    ret

benchInsertion:
    ld a,(ncSortCount)
    cp 2
    ret c
    ld a,1
    ld (ncSortGap),a
    jp ncSortPass

; Quicksort: middle pivot, Lomuto partition; comparator includes source index.
benchQuick:
    ld a,(ncSortCount)
    cp 2
    ret c
    dec a
    ld c,a
    ld b,0
benchQuickRange:
    ld a,b
    cp c
    ret nc
    push bc
    ld a,c
    sub b
    or a
    rra
    add a,b
    ld (benchPivot),a
    call ncIndexAddress
    ld d,(hl)
    ld a,c
    call ncIndexAddress
    ld e,(hl)
    ld (hl),d
    ld a,(benchPivot)
    call ncIndexAddress
    ld (hl),e
    ld a,d
    ld (ncSortKey),a
    call ncSortFileAtIndex
    ld (ncSortKeyPtr),hl
    ld a,b
    ld (benchI),a
    ld (benchJ),a
    ld a,c
    ld (benchEnd),a
benchQuickPartition:
    ld a,(benchEnd)
    ld b,a
    ld a,(benchJ)
    cp b
    jp z,benchQuickPivot
    call ncIndexAddress
    ld a,(hl)
benchQuickCompare:
    call ncCompareNameFiles
    jp c,benchQuickSwap
    jp nz,benchQuickNext
benchQuickSwap:
    call benchSwapIJ
    ld hl,benchI
    inc (hl)
benchQuickNext:
    ld hl,benchJ
    inc (hl)
    jp benchQuickPartition
benchQuickPivot:
    ; J is the pivot's old position; place it between both partitions.
    call benchSwapIJ
    pop bc
    ld a,(benchI)
    push af
    push bc
    cp b
    jp z,benchQuickRight
    dec a
    ld c,a
    call benchQuickRange
benchQuickRight:
    pop bc
    pop af
    cp c
    ret nc
    inc a
    ld b,a
    jp benchQuickRange
benchSwapIJ:
    ld a,(benchJ)
    call ncIndexAddress
    ld d,(hl)
    ld a,(benchI)
    call ncIndexAddress
    ld e,(hl)
    ld (hl),d
    ld a,(benchJ)
    call ncIndexAddress
    ld (hl),e
    ret

; Bottom-up stable merge of byte indices. Original FILE_INFO stays intact.
benchMerge:
    ld a,1
    ld (benchWidth),a
benchMergePass:
    ld a,(ncSortCount)
    ld b,a
    ld a,(benchWidth)
    cp b
    ret nc
    xor a
    ld (benchLeft),a
benchMergeBlock:
    ld a,(benchLeft)
    ld (benchI),a
    ld (benchK),a
    call benchBound
    ld (benchMid),a
    ld (benchJ),a
    call benchBound
    ld (benchEnd),a
benchMergeNext:
    ld a,(benchMid)
    ld b,a
    ld a,(benchI)
    cp b
    jp z,benchTakeRight
    ld a,(benchEnd)
    ld b,a
    ld a,(benchJ)
    cp b
    jp z,benchTakeLeft
    call ncIndexAddress
    ld a,(hl)
    ld (ncSortKey),a
    call ncSortFileAtIndex
    ld (ncSortKeyPtr),hl
    ld a,(benchI)
    call ncIndexAddress
    ld a,(hl)
benchMergeCompare:
    call ncCompareNameFiles
    jp c,benchTakeLeft
    jp z,benchTakeLeft
benchTakeRight:
    ld hl,benchJ
    jp benchTake
benchTakeLeft:
    ld hl,benchI
benchTake:
    ld a,(hl)
    inc (hl)
    call ncIndexAddress
    ld b,(hl)
    ld a,(benchK)
    ld l,a
    ld h,benchScratch/256
    ld (hl),b
    inc a
    ld (benchK),a
    ld hl,benchEnd
    cp (hl)
    jp c,benchMergeNext
    ld (benchLeft),a
    ld hl,ncSortCount
    cp (hl)
    jp c,benchMergeBlock
    call benchCopyIndices
    ld a,(benchWidth)
    add a,a
    ret c
    ld (benchWidth),a
    jp benchMergePass
benchBound:
    ld hl,benchWidth
    add a,(hl)
    jp c,benchBoundCount
    ld hl,ncSortCount
    cp (hl)
    ret c
benchBoundCount:
    ld a,(ncSortCount)
    ret
benchCopyIndices:
    ld hl,benchScratch
    ld de,FILE_INDEX_A
    ld a,(ncSortCount)
    ld b,a
    or a
    ret z
benchCopyLoop:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,benchCopyLoop
    ret

; Stable LSD counting sort: 11 filename bytes and directory rank, no strcmp.
benchRadix:
    ld a,(ncSortCount)
    cp 2
    ret c
    ld hl,benchNameOrder
    ld a,(ncSortMode)
    cp 2
    jp nz,benchRadixOrder
    ld hl,benchExtOrder
benchRadixOrder:
    ld (benchOrder),hl
benchRadixPass:
    ld hl,(benchOrder)
    ld a,(hl)
    cp 0FFh
    ret z
    inc hl
    ld (benchOrder),hl
    ld (benchOffset),a
    ld hl,benchCounts
    ld b,0
    xor a
benchZeroCounts:
    ld (hl),a
    inc l
    dec b
    jp nz,benchZeroCounts
    xor a
    ld (benchI),a
benchCountKeys:
    call benchRadixKey
    ld l,a
    ld h,benchCounts/256
    inc (hl)
    ld hl,benchI
    inc (hl)
    ld a,(ncSortCount)
    cp (hl)
    jp nz,benchCountKeys
    ld hl,benchCounts
    ld b,0
    ld c,0
benchPrefixCounts:
    ld a,(hl)
    ld (hl),c
    add a,c
    ld c,a
    inc l
    dec b
    jp nz,benchPrefixCounts
    xor a
    ld (benchI),a
benchDistribute:
    call benchRadixKey
    ld l,a
    ld h,benchCounts/256
    ld c,(hl)
    inc (hl)
    ld a,(benchI)
    call ncIndexAddress
    ld a,(hl)
    ld h,benchScratch/256
    ld l,c
    ld (hl),a
    ld hl,benchI
    inc (hl)
    ld a,(ncSortCount)
    cp (hl)
    jp nz,benchDistribute
    call benchCopyIndices
    jp benchRadixPass
benchRadixKey:
    ld a,(benchI)
    call ncIndexAddress
    ld a,(hl)
    call fileInfoAtIndex
    IF PREUPPER == 0
    ld a,(benchOffset)
    cp 12
    jp z,ncFileRank
    ENDIF
    ld a,(benchOffset)
    add a,l
    ld l,a
    jp nc,benchRadixKeyLoad
    inc h
benchRadixKeyLoad:
    ld a,(hl)
    IF PREUPPER == 0
    jp bios_upperCase
    ELSE
    ret
    ENDIF

fileCacheBase:
    IF PREUPPER
    ld hl,benchNormalized
    ELSE
    ld hl,benchRaw
    ENDIF
    ret
readBytePanel:
    ld a,(hl)
    ret

state:
.activePanel: DB 0
.panelA_filesCnt: DB 0
.panelA_sort: DB 2
benchNameOrder: DB 10,9,8,7,6,5,4,3,2,1,0,12,0FFh
benchExtOrder: DB 7,6,5,4,3,2,1,0,10,9,8,12,0FFh
benchOrder: DW 0
benchOffset: DB 0
benchStart: DW 0
benchElapsed: DW 0
benchRank: DB 0
benchWidth: DB 0
benchLeft: DB 0
benchMid: DB 0
benchEnd: DB 0
benchI: DB 0
benchJ: DB 0
benchK: DB 0
benchPivot: DB 0
benchDivisorHigh: DB 1

    INCLUDE "@TIMING@"
    INCLUDE "@CORE@"
    ASSERT $ < benchRaw
