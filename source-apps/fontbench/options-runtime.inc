; Постоянные фильтры; разбор аргументов временно занимает фазу 4 таблицы.
benchCaseSelected:
    push hl
    push bc
    push de
    call benchIsKernel
    inc a
    ld d,a
    call benchIsPaged
    jp z,benchMaskReady
    IFDEF FONT_PAGE6
    push hl
    inc hl
    inc hl
    ld a,(hl)
    pop hl
    cp 3
    jp nc,benchCaseNo
    ENDIF
    ld d,4
benchMaskReady:
    ld a,(benchEngine)
    and d
    jp z,benchCaseNo
    inc hl
    inc hl
    ld c,(hl)
    inc hl
    ld b,(hl)
    ld d,1
    ld a,c
    cp 3
    jp c,benchFontMask
    ld d,2
    cp 5
    jp c,benchFontMask
    ld d,4
benchFontMask:
    ld a,(benchFont)
    and d
    jp z,benchCaseNo
    ld a,c
    cp 5
    jp c,benchCallMask
    ld d,1
    cp 7
    jp c,benchPackedMask
    ld d,2
    cp 9
    jp c,benchPackedMask
    ld d,4
benchPackedMask:
    ld a,(benchPacked)
    and d
    jp z,benchCaseNo
benchCallMask:
    ld d,4
    ld a,b
    and 16
    jp nz,benchCallCheck
    ld d,8
    ld a,b
    and 64
    jp nz,benchCallCheck
    ld d,16
    ld a,b
    and 32
    jp nz,benchCallCheck
    ld d,32
    ld a,c
    cp 1
    jp z,benchCallCheck
    ld d,1
    or a
    jp z,benchCallCheck
    cp 3
    jp z,benchCallCheck
    cp 5
    jp c,benchCallSerial
    and 1
    jp nz,benchCallCheck
benchCallSerial:
    ld d,2
benchCallCheck:
    ld a,(benchCalls)
    and d
    jp z,benchCaseNo
    ld a,(benchBanks)
    cp 2
    jp nz,benchCaseYes
    dec hl
    dec hl
    dec hl
    call benchBankSupported
    jp z,benchCaseNo
benchCaseYes:
    ld a,1
    jp benchCaseReturn
benchCaseNo:
    xor a
benchCaseReturn:
    pop de
    pop bc
    pop hl
    or a
    ret
benchCountSelected:
    xor a
    ld (benchStageTotal),a
    ld hl,matrixCases
    ld b,0
benchCountCase:
    ld a,(hl)
    inc hl
    or (hl)
    dec hl
    jp z,benchCountDone
    call benchCaseSelected
    jp z,benchCountNext
    inc b
    push bc
    ld a,(benchBanks)
    and 1
    ld c,a
    ld a,(benchBanks)
    and 2
    jp z,benchCountAdd
    call benchBankSupported
    jp z,benchCountAdd
    inc c
benchCountAdd:
    ld a,(benchStageTotal)
    add a,c
    ld (benchStageTotal),a
    pop bc
benchCountNext:
    ld de,14
    add hl,de
    jp benchCountCase
benchCountDone:
    ld a,b
    ld (benchCaseCount),a
    ld a,(benchStageTotal)
    or a
    ret
; Все standalone-движки доступны через подменённый штатный PRINT_STRING_BANK.
benchBankSupported:
    ld a,1
    or a
    ret
benchOptions:
    IFDEF FONT_PAGE6
benchFont: DB 1
    ELSE
benchFont: DB 7
    ENDIF
benchPacked: DB 7
benchBanks: DB 3
benchCalls: DB 63
benchNoSave: DB 0
benchQuick: DB 0
benchHelp: DB 0
    IFDEF FONT_PAGE6
benchEngine: DB 4
    ELSE
benchEngine: DB 7
    ENDIF
benchCaseCount: DB MATRIX_CASE_COUNT
benchStageTotal: DB MATRIX_STAGE_COUNT
benchMissingText: DB '-',0
benchSkippedText: DB 'Report disabled',13,10,0

benchIsPaged:
    push hl
    push de
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld de,pagedCoreStart
    call bios_cmp_hl_de
    jp c,benchPagedNo
    ld de,pagedCoreEnd
    call bios_cmp_hl_de
    jp nc,benchPagedNo
    ld a,1
    jp benchPagedDone
benchPagedNo:
    xor a
benchPagedDone:
    pop de
    pop hl
    or a
    ret
