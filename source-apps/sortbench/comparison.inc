; Индексы 0..254 с FF-терминатором; сами FILE_INFO не переставляются.
; Входы сортировки работают с активной панелью. AF/BC/DE/HL разрушаются.
fileIndexBase:
    ld hl,FILE_INDEX_A
    ld a,(state.activePanel)
    or a
    ret z
    ld hl,FILE_INDEX_B
    ret

; A=номер исходной записи; BC/DE сохранены, AF/HL разрушаются.
fileInfoAtIndex:
    push de
    ld l,a
    ld h,0
    add hl,hl
    add hl,hl
    add hl,hl
    add hl,hl
    ex de,hl
    call fileCacheBase
    add hl,de
    pop de
    ret

; HL=элемент индексной таблицы. FF указывает на терминатор кэша.
; BC/DE сохранены; HL=FILE_INFO, AF разрушается.
indexedFile:
    ld a,(hl)
    cp 0FFh
    jp nz,fileInfoAtIndex
    ld hl,state.panelA_filesCnt
    call readBytePanel
    jp fileInfoAtIndex

; При чтении каталога и смене режима начинаем с порядка на диске.
buildFileIndex:
    call fileIndexBase
    ld (ncSortBase),hl
    push hl
    ld hl,state.panelA_filesCnt
    call readBytePanel
    ld (ncSortCount),a
    ld b,a
    ld c,0
    pop hl
    or a
    jp z,ncIndexTerminator
ncIndexFill:
    ld (hl),c
    inc hl
    inc c
    dec b
    jp nz,ncIndexFill
ncIndexTerminator:
    ld (hl),0FFh
    ld hl,state.panelA_sort
    call readBytePanel
    ld (ncSortMode),a
    or a
    ret z
    call ncSortSetup
    IF NC_SORT_RADIX
    call ncRadixPrepare
    jp ncRadixSort
    ELSE
    call ncPrepareRanks
    ld a,(ncSortCount)
    or a
    rra
    ld (ncSortGap),a
    ENDIF
; Shell: gap=N/2,...,1. На шаге перемещаются только байтовые индексы.
ncSortPass:
    ld a,(ncSortGap)
    or a
    ret z
    ld (ncSortI),a
ncSortItem:
    ld a,(ncSortI)
    ld (ncSortJ),a
    call ncIndexAddress
    ld a,(hl)
    ld (ncSortKey),a
    call ncSortFileAtIndex
    ld (ncSortKeyPtr),hl
ncSortInsert:
    ld a,(ncSortJ)
    ld hl,ncSortGap
    sub (hl)
    jp c,ncSortPlace
    ld (ncSortPrev),a
    call ncIndexAddress
    ld a,(hl)
ncSortCompare:
    call ncCompareNameFiles
    jp c,ncSortPlace
    jp z,ncSortPlace
    ld a,(ncSortJ)
    call ncIndexAddress
    ld a,(ncSortLeft)
    ld (hl),a
    ld a,(ncSortPrev)
    ld (ncSortJ),a
    jp ncSortInsert
ncSortPlace:
    ld a,(ncSortJ)
    call ncIndexAddress
    ld a,(ncSortKey)
    ld (hl),a
    ld hl,ncSortI
    inc (hl)
    ld a,(hl)
    ld hl,ncSortCount
    cp (hl)
    jp c,ncSortItem
    ld a,(ncSortGap)
    or a
    rra
    ld (ncSortGap),a
    jp ncSortPass

; A=позиция, HL=ячейка индекса; DE сохранён.
ncIndexAddress:
    ld hl,(ncSortBase)
    add a,l
    ld l,a
    ret nc
    inc h
    ret

; Cache panel-dependent addressing and patch the direct comparator CALL once.
ncSortSetup:
    call fileCacheBase
    ld (ncSortRecords),hl
    ld hl,ncCompareNameFiles
    ld a,(ncSortMode)
    cp 2
    jp nz,ncSortComparatorReady
    ld hl,ncCompareExtFiles
ncSortComparatorReady:
    ld (ncSortComparator),hl
    ld (ncSortCompare+1),hl
    ret
ncSortFileAtIndex:
    push de
    ld l,a
    ld h,0
    add hl,hl
    add hl,hl
    add hl,hl
    add hl,hl
ncSortRecordLoad:
    ld de,0
    add hl,de
    pop de
    ret

; Rank is calculated once per record. Validate padding once, retaining full
; bytewise comparison for a foreign entry with an internal space.
ncPrepareRanks:
    ld a,1
    ld (ncSortPaddingSafe),a
    ld a,(ncSortCount)
    or a
    ret z
    ld b,a
    ld hl,(ncSortRecords)
    ld e,0
ncPrepareRankRow:
    call ncFileRank
    ld d,NC_SORT_RANKS/256
    ld (de),a
    push hl
    push bc
    ld b,8
    call ncCheckPaddingField
    ld b,3
    call ncCheckPaddingField
    pop bc
    pop hl
    push bc
    ld bc,16
    add hl,bc
    pop bc
    inc e
    dec b
    jp nz,ncPrepareRankRow
    ret
ncCheckPaddingField:
    ld c,0
ncCheckPaddingByte:
    ld a,(hl)
    cp ' '
    jp nz,ncCheckPaddingNonSpace
    ld c,1
    jp ncCheckPaddingNext
ncCheckPaddingNonSpace:
    ld a,c
    or a
    jp z,ncCheckPaddingNext
    xor a
    ld (ncSortPaddingSafe),a
ncCheckPaddingNext:
    inc hl
    dec b
    jp nz,ncCheckPaddingByte
    ret

    MACRO NC_COMPARE_START
    ld (ncSortLeft),a
    push bc
    push de
    push hl
    call ncSortFileAtIndex
    push hl
    ld hl,(ncSortKeyPtr)
    ex de,hl
    ld a,(ncSortLeft)
    ld l,a
    ld h,NC_SORT_RANKS/256
    ld c,(hl)
    ld a,(ncSortKey)
    ld l,a
    ld b,(hl)
    ld a,c
    cp b
    pop hl
    jp nz,ncCompareDone
    ENDM
ncCompareNameFiles:
    NC_COMPARE_START
    ld b,8
    call ncComparePart
    jp nz,ncCompareDone
    ld b,3
    call ncComparePart
    jp ncCompareTie
ncCompareExtFiles:
    NC_COMPARE_START
    push hl
    push de
    ld bc,8
    add hl,bc
    ex de,hl
    add hl,bc
    ex de,hl
    ld b,3
    call ncComparePart
    pop de
    pop hl
    jp nz,ncCompareDone
    ld b,8
    call ncComparePart
ncCompareTie:
    jp nz,ncCompareDone
    ld a,(ncSortKey)
    ld c,a
    ld a,(ncSortLeft)
    cp c
ncCompareDone:
    pop hl
    pop de
    pop bc
    ret
ncComparePart:
    ld a,(de)
    ld c,a
    ld a,(hl)
    cp c
    jp nz,ncCompareDifferent
    cp ' '
    jp nz,ncCompareNext
    ld a,(ncSortPaddingSafe)
    or a
    jp nz,ncComparePadding
    jp ncCompareNext
ncCompareDifferent:
    ld a,c
    call bios_upperCase
    ld c,a
    ld a,(hl)
    call bios_upperCase
    cp c
    ret nz
ncCompareNext:
    inc hl
    inc de
    dec b
    jp nz,ncComparePart
    ret
ncComparePadding:
    ; Advance to the next fixed-width field without reading the padding.
    inc hl
    inc de
    dec b
    jp nz,ncComparePadding
    ret

ncFileRank:
    push hl
    push bc
    ld bc,FILE_INFO.attrib
    add hl,bc
    ld a,(hl)
    and 010h
    pop bc
    pop hl
    ld a,3
    ret z
    ld a,(hl)
    cp 05Ch
    jp nz,ncRankParent
    inc hl
    ld a,(hl)
    dec hl
    cp ' '
    ld a,0
    ret z
ncRankParent:
    ld a,(hl)
    cp '.'
    jp nz,ncRankDirectory
    inc hl
    ld a,(hl)
    cp '.'
    jp nz,ncRankNotParent
    inc hl
    ld a,(hl)
    dec hl
    cp ' '
ncRankNotParent:
    dec hl
    ld a,1
    ret z
ncRankDirectory:
    ld a,2
    ret

ncSortBase: DW 0
ncSortKeyPtr: DW 0
ncSortCount: DB 0
ncSortMode: DB 0
ncSortGap: DB 0
ncSortI: DB 0
ncSortJ: DB 0
ncSortPrev: DB 0
ncSortKey: DB 0
ncSortLeft: DB 0
ncSortRecords = ncSortRecordLoad+1
ncSortComparator: DW 0
ncSortPaddingSafe: DB 0

    INCLUDE "sort-radix.inc"
