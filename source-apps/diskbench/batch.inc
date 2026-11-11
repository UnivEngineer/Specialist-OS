; Пакет MAIN/DOS/BANK; все записи получены до печати таблицы.
batchCollect:
    ld hl,(batchNext)
    ex de,hl
    ld hl,resultsLow
    ld b,40
    call copy_bytes
    ld hl,statRawVerify
    ld b,20
    call copy_bytes
    ex de,hl
    ld (batchNext),hl
    ld hl,argBank
    dec (hl)
    jp nz,moduleSelect
    call printBatch
    jp done

; Синтетическая запись только для печати: WRITE + отдельный VERIFY.
makeVerified:
    ld de,verifiedStat
    ld b,10
    call copy_bytes
    ld hl,verifiedStat
    ex de,hl
verifySource:
    ld hl,statFileVerify
    ld b,4
    jp add_bytes
printRawVerified:
    ld a,(v_layout)
    or a
    jp z,printRawLowVerified
    ld hl,statRawVerifyHigh
    ld (verifySource+1),hl
    ld hl,resultsHigh
    call makeVerified
    ld de,txtRawHighVerified
    call printVerifiedLabel
printRawLowVerified:
    ld hl,statRawVerify
    ld (verifySource+1),hl
    ld hl,resultsLow
    call makeVerified
    ld de,txtWriteVerified
    ld a,(v_layout)
    or a
    jp z,printVerifiedLabel
    ld de,txtRawLowVerified
    jp printVerifiedLabel
printWriteVerified:
    ld hl,statFileVerify
    ld (verifySource+1),hl
    ld hl,statFileWrite
    call makeVerified
printVerified:
    ld de,txtWriteVerified
printVerifiedLabel:
    ld hl,verifiedStat
    jp print_stat

printBatch:
    ld hl,txtBatchHeader
    call puts
    ld a,(argMode)
    and 1
    jp z,batchReadOnly
    ld a,(v_drive)
    or a
    jp nz,batchReadLow
    ld a,52
    ld (batchOffset),a
    ld hl,txtBatchRawReadHigh
    call batchRow
batchReadLow:
    ld a,10
    ld (batchOffset),a
    ld hl,txtBatchRawRead
    ld a,(v_drive)
    or a
    jp nz,batchReadLabel
    ld hl,txtBatchRawReadLow
batchReadLabel:
    call batchRow
batchWrite:
    ld a,(v_budget)
    or a
    jp z,batchReadOnly
    ld a,(v_drive)
    or a
    jp nz,batchWriteLow
    ld a,48
    ld (batchOffset),a
    ld hl,txtBatchRawWriteHigh
    call batchRow
batchWriteLow:
    xor a
    ld (batchOffset),a
    ld hl,txtBatchRawWrite
    ld a,(v_drive)
    or a
    jp nz,batchWriteLabel
    ld hl,txtBatchRawWriteLow
batchWriteLabel:
    call batchRow
    ld a,(v_writeMode)
    cp 6
    jp nz,batchReadOnly
    ld a,(v_drive)
    or a
    jp nz,batchVerifyLow
    ld a,64
    ld (batchOffset),a
    ld hl,txtBatchRawHighVerify
    call batchRow
batchVerifyLow:
    ld a,60
    ld (batchOffset),a
    ld hl,txtBatchRawVerify
    ld a,(v_drive)
    or a
    jp nz,batchVerifyLabel
    ld hl,txtBatchRawLowVerify
batchVerifyLabel:
    call batchRow
batchReadOnly:
    ld a,(argMode)
    and 2
    ret z
    ld a,30
    ld (batchOffset),a
    ld hl,txtBatchDosRead
    call batchRow
    ld a,(v_budget)
    or a
    ret z
    ld a,20
    ld (batchOffset),a
    ld hl,txtBatchDosWrite
    call batchRow
    ld a,(v_writeMode)
    cp 6
    ret nz
    ld a,40
    ld (batchOffset),a
    ld hl,txtBatchDosVerify
batchRow:
    call puts
    ld hl,batchStats
    ld (batchCell),hl
    ld a,3
    ld (batchColumns),a
batchColumn:
    ld hl,(batchCell)
    ld a,(batchOffset)
    cp 64
    jp nz,batchCheckLowSum
    ld de,48
    add hl,de
    jp batchRawSum
batchCheckLowSum:
    cp 60
    jp z,batchRawSum
    cp 40
    jp nz,batchPlain
    ld de,20
    add hl,de
batchRawSum:
    ld de,v_elapsed
    ld b,4
    call copy_bytes
    ld hl,(batchCell)
    ld de,44
    ld a,(batchOffset)
    cp 64
    jp nz,batchLowVerifyOffset
    ld e,56
    jp batchVerifyOffset
batchLowVerifyOffset:
    cp 60
    jp nz,batchVerifyOffset
    ld e,40
batchVerifyOffset:
    add hl,de
    ld de,v_elapsed
    ld b,4
    call add_bytes
    jp batchRate
batchPlain:
    ld e,a
    ld d,0
    add hl,de
    ld de,v_elapsed
    ld b,4
    call copy_bytes
batchRate:
    ld c,9
    call bench_printChar
    ld hl,(v_elapsed)
    LOAD_DE v_elapsed+2
    ld a,h
    or l
    or d
    or e
    jp z,batchMissing
    call calculate_rate
    call print_rate_value
    jp batchAdvance
batchMissing:
    ld hl,txtBatchMissing
    call puts
batchAdvance:
    ld hl,(batchCell)
    ld de,60
    add hl,de
    ld (batchCell),hl
    ld hl,batchColumns
    dec (hl)
    jp nz,batchColumn
    ld hl,txtLineBreak
    jp puts
batchEnabled: DB 0
moduleCheckMode: DB 0
batchNext: DW 0
batchCell: DW 0
batchColumns: DB 0
batchOffset: DB 0
verifiedStat: DS 10,0
; MAIN выполняется первым. После MODULE_UNLOAD его арена 2000h свободна;
; DOS/BANK не пишут в неё. До завершения MAIN пакетных результатов ещё нет.
batchStats = 02000h
    ASSERT batchStats+3*60 <= 02C00h
txtRawHighVerified: DB "WRITE HIGH + VERIFY:",9,0
txtRawLowVerified: DB "WRITE LOW + VERIFY:",9,0
txtWriteVerified: DB "WRITE + VERIFY:",9,0
txtBatchHeader: DB 10,10,"KiB/s       ",9,"       MAIN",9,"        DOS",9,"       BANK",10,0
txtBatchRawRead: DB "Raw READ    ",0
txtBatchRawWrite: DB "Raw WRITE   ",0
txtBatchRawReadLow: DB "READ LOW    ",0
txtBatchRawWriteLow: DB "WRITE LOW   ",0
txtBatchRawReadHigh: DB "READ HIGH   ",0
txtBatchRawWriteHigh: DB "WRITE HIGH  ",0
txtBatchRawHighVerify: DB "HIGH W+VFY  ",0
txtBatchRawLowVerify: DB "LOW W+VERIFY",0
txtBatchRawVerify: DB "Raw W+VERIFY",0
txtBatchDosRead: DB "FILE READ   ",0
txtBatchDosWrite: DB "FILE WRITE  ",0
txtBatchDosVerify: DB "FILE W+VERIFY",0
txtBatchMissing: DB "        -- ",0
