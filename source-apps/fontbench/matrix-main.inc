; Единый корпус: сначала main, затем отдельный EXEC-банк.
entry:
    push de
    call bench_init
    pop hl
    jp c,bench_finish
    call benchParseArgs
    jp c,matrixArgsBad
    ld a,(benchHelp)
    or a
    jp nz,matrixUsage
    call benchCountSelected
    jp z,matrixArgsBad
    ; Возвращаем фазу 4 только после выхода из временного кода парсера.
    ld a,1
    ld (packedOverlay),a
    call font4RestorePackedOverlay
    call bios_getMemTop
    ld de,textCorpus+FONT_CORPUS_SIZE-1
    call bios_cmp_hl_de
    jp c,corpusNoMemory
    ld hl,07E00h
    ld (cachePrivateStart),hl
    ld hl,08600h
    ld (cachePrivateEnd),hl
    call loadCorpus
    jp c,corpusFailed
    IFDEF FONT_BLOCK32
    call blockNormalizeCorpus
    ENDIF
    ld a,(benchBanks)
    and 2
    call nz,bank_open
    jp c,matrixModuleFailed
    ld hl,FONT_ADDR
    ld (bios_upper.charGen),hl
    ld hl,monitor.drawEntry
    ld de,originalDraw
    ld bc,3
    call bench_copy
    ld c,'A'
    call bios_printChar
    call bench_clear
report:
    ld hl,matchText
    ld (bitmapResult),hl
    xor a
    ld (matrixBank),a
    ld (matrixStage),a
    ld (bitmapFailedStage),a
matrixDirection:
    ld a,(matrixBank)
    inc a
    ld b,a
    ld a,(benchBanks)
    and b
    jp z,matrixDirectionDone
    call font4RestorePhase0
    ld hl,matrixCases
matrixNext:
    push hl
    call pagedRestore
    pop hl
    push hl
    call font4RestorePackedOverlay
    pop hl
    ld (matrixCase),hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    ld a,d
    or e
    jp z,matrixDirectionDone
    dec hl
    call benchCaseSelected
    jp z,matrixSkipCase
    ld a,(matrixBank)
    or a
    jp z,matrixCaseReady
    call benchBankSupported
    jp z,matrixSkipCase
matrixCaseReady:
    inc hl
    push hl
    ex de,hl
    ld (output+1),hl
    ld (quadOutput+1),hl
    ld (serialOutput+1),hl
    pop hl
    inc hl
    ld a,(hl)
    ld (matrixKind),a
    inc hl
    ld a,(hl)
    ld (matrixFlags),a
    call restoreOriginal
    ld a,(matrixFlags)
    and 4
    call nz,installOptimized
    ld a,(matrixFlags)
    and 1
    call nz,cacheDiscard
    ld a,(matrixFlags)
    and 2
    call nz,cacheResetStats
    ld a,(matrixFlags)
    and 8
    call nz,quadPrepareHalf
    ld a,(matrixFlags)
    and 128
    call nz,quadPreparePacked
    ld a,(matrixKind)
    cp 5
    jp c,matrixFont4Ready
    sub 5
    or a
    rra
    call font4Prepare
matrixFont4Ready:
    call pagedSelect
    ld hl,(matrixCase)
    call benchIsKernel
    call nz,kernelPrepare
    call streamPrepare
    call matrixClearText
    call streamInstall
    ld a,(matrixBank)
    or a
    jp nz,matrixRunBank
    ld a,(matrixKind)
    or a
    jp z,matrixRunScalar
    cp 3
    jp c,matrixNotAligned
    and 1
    jp nz,matrixRunScalar
    jp matrixRunSerial
matrixNotAligned:
    dec a
    jp z,matrixRunQuad
matrixRunSerial:
    call renderSerialScreen
    jp matrixMeasured
matrixRunScalar:
    call renderScreen
    jp matrixMeasured
matrixRunQuad:
    call renderQuadScreen
    jp matrixMeasured
matrixRunBank:
    call bank_run
    jp c,matrixModuleFailedRestore
matrixMeasured:
    call streamRestore
    ld hl,(matrixCase)
    ld de,4
    add hl,de
    ld a,(matrixBank)
    or a
    jp z,matrixTimeAddress
    inc hl
    inc hl
matrixTimeAddress:
    ld e,(hl)
    inc hl
    ld d,(hl)
    ld hl,v_elapsed
    ld bc,4
    call bench_copy
    ld hl,(matrixCase)
    ld de,8
    add hl,de
    ld a,(matrixBank)
    or a
    jp z,matrixMissAddress
    inc hl
    inc hl
matrixMissAddress:
    ld e,(hl)
    inc hl
    ld d,(hl)
    ld a,d
    or e
    jp z,matrixChecked
    ld hl,(cacheMisses)
    ex de,hl
    ld (hl),e
    inc hl
    ld (hl),d
matrixChecked:
    call checkBankFrame
    call matrixProgress
    ld hl,(matrixCase)
    ld de,14
    add hl,de
    jp matrixNext
matrixSkipCase:
    ld de,14
    add hl,de
    jp matrixNext
matrixDirectionDone:
    ld a,(matrixBank)
    inc a
    ld (matrixBank),a
    cp 2
    jp nz,matrixDirection
    call restoreOriginal
    call cacheDiscard
    ld hl,benchSkippedText
    ld (reportMessage),hl
    ld a,(benchNoSave)
    or a
    call z,matrixSaveReport
    ld a,(benchQuick)
    or a
    jp nz,matrixQuickDone
    call bench_clear
    ld hl,title
    call puts
    ld a,15
    ld (matrixSummaryY),a
    ld hl,matrixCases
matrixPrint:
    ld (matrixCase),hl
    ld a,(hl)
    inc hl
    or (hl)
    jp z,matrixPrinted
    dec hl
    call benchCaseSelected
    jp z,matrixPrintSkip
    inc hl
    push hl
    ld a,(matrixSummaryY)
    cp 215
    jp c,matrixPageReady
    ld hl,matrixPageText
    call puts
matrixPageKey:
    call bios_getch
    cp 13
    jp nz,matrixPageKey
    call bench_clear
    ld hl,title
    call puts
    ld a,15
    ld (matrixSummaryY),a
matrixPageReady:
    ld a,(matrixSummaryY)
    ld l,a
    ld h,0
    call bios_setCursorPos
    ld a,(matrixSummaryY)
    add a,8
    ld (matrixSummaryY),a
    pop hl
    call matrixPrintValues
matrixPrintSkip:
    ld hl,(matrixCase)
    ld de,14
    add hl,de
    jp matrixPrint
matrixPrintValues:
    ld hl,(matrixCase)
    ld de,12
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    call puts
    ld a,' '
    call timing_printChar
    ld hl,(matrixCase)
    ld de,4
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld a,(benchBanks)
    and 1
    jp z,matrixMissingMain
    call print_time_ms
    jp matrixMainPrinted
matrixMissingMain:
    ld hl,benchMissingText
    call puts
matrixMainPrinted:
    ld hl,slashText
    call puts
    ld hl,(matrixCase)
    ld de,6
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld a,(benchBanks)
    and 2
    jp z,matrixMissingDOS
    push hl
    ld hl,(matrixCase)
    call benchBankSupported
    pop hl
    jp z,matrixMissingDOS
    call print_time_ms
    jp matrixDOSPrinted
matrixMissingDOS:
    ld hl,benchMissingText
    call puts
matrixDOSPrinted:
    ld hl,newlineText
    call puts
    ret
matrixPrinted:
    ld a,(matrixSummaryY)
    ld l,a
    ld h,0
    call bios_setCursorPos
matrixResultsText:
    call bank_close
    ld hl,(bitmapResult)
    call puts
    ld hl,(reportMessage)
    call puts
    jp bench_finish

checkBankFrame:
    ld a,(matrixKind)
    cp 5
    jp nc,checkFont4Frame
    cp 3
    jp nc,checkAligned8Frame
    jp checkFont6Frame
; Эталон 6x8 теперь вычисляется прямо из исходного шрифта и корпуса.
; Прежние 6912 байт снимка освобождены под код/матрицу 4x8.
saveReference:
    ret
bankMismatch:
    ld a,(bitmapFailedStage)
    or a
    jp nz,bankMismatchKnown
    ld a,(matrixStage)
    inc a
    ld (bitmapFailedStage),a
bankMismatchKnown:
    ld hl,failText
    ld (bitmapResult),hl
    ret

renderScreen:
    call reset_timing
    ld hl,bankImage
streamScalarNext:
    ld a,(hl)
    or a
    jp z,timer_sample
    ld c,a
    push hl
    ; PRINT_CHAR_REAL не разбирает CRLF: управляющие идут обычным API.
    cp 020h
    jp c,streamScalarNormal
    ld a,(streamReal)
    or a
    jp z,streamScalarNormal
    ld a,c
    MXOS_CALL MXOS_BIOS_PRINT_CHAR_REAL
    jp streamScalarDone
streamScalarNormal:
    MXOS_CALL MXOS_BIOS_PRINT_CHAR
streamScalarDone:
    pop hl
    inc hl
    jp streamScalarNext
renderQuadScreen:
renderSerialScreen:
    call reset_timing
    ld hl,bankImage
    MXOS_CALL MXOS_BIOS_PRINT_STRING_MAIN
    jp timer_sample
    INCLUDE "stream.inc"
matrixArgsBad:
    ld hl,benchInvalidText
    call puts
matrixUsage:
    ld hl,benchUsage
    call puts
    jp bench_restore
matrixQuickDone:
    ld hl,newlineText
    call puts
    jp matrixResultsText
corpusNoMemory:
    ld hl,corpusMemoryText
    call puts
    jp bench_finish
corpusFailed:
    ld hl,corpusErrorText
    call puts
    jp bench_finish
matrixModuleFailedRestore:
    call streamRestore
    call restoreOriginal
    call cacheDiscard
matrixModuleFailed:
    call bank_close
    ld hl,bankErrorText
    call puts
    jp bench_finish
title: DB 'FONTBEN README 48x24, ms: main / BANK',13,10,0
corpusMemoryText: DB 'Need main RAM through 8AAFh',13,10,0
corpusErrorText: DB 'Cannot read A:\README.TXT',13,10,0
matchText: DB 'Bitmap: PASS',13,10,0
failText: DB 'Bitmap: FAIL',13,10,0
newlineText: DB 13,10,0
slashText: DB ' / ',0
mainRows: DB 0
matrixBank: DB 0
matrixKind: DB 0
matrixFlags: DB 0
matrixSummaryY: DB 0
matrixCase: DW 0
bitmapResult: DW 0
bitmapFailedStage: DB 0
