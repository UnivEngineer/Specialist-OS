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
    call fileCacheBase
    ld (ncSortRecords),hl
    call ncRadixPrepare
    jp ncRadixSort
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
ncSortCount: DB 0
ncSortMode: DB 0
ncSortRecords: DW 0

    INCLUDE "sort-radix.inc"
