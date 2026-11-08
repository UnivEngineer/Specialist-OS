; Единый корпус и таблица вариантов: сначала все main, затем все DOS.
entry:
    call bench_init
    jp c,bench_finish
    call bios_getMemTop
    ld de,textCorpus+FONT_CORPUS_SIZE-1
    call bios_cmp_hl_de
    jp c,corpusNoMemory
    call loadCorpus
    jp c,corpusFailed
    ld hl,textCorpus
    ld de,bankImage+0200h
    ld bc,FONT_CORPUS_SIZE
    call bench_copy
    ld hl,02800h/8
    ld (bios_vars.charGen),hl
    ld hl,BASE_DRAWCHAR
    ld de,originalDraw
    ld bc,3
    call bench_copy
    ld c,'A'
    call bios_printChar
    call bench_clear
    ld hl,bios_printChar
    ld (output+1),hl
    call renderScreen
    call saveReference
report:
    ld hl,matchText
    ld (bitmapResult),hl
    xor a
    ld (matrixBank),a
    ld (matrixStage),a
matrixDirection:
    ld hl,matrixCases
matrixNext:
    ld (matrixCase),hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    ld a,d
    or e
    jp z,matrixDirectionDone
    push hl
    ex de,hl
    ld (output+1),hl
    ld (bankOutput+1),hl
    ld (quadOutput+1),hl
    ld (bankQuadOutput+1),hl
    ld (serialOutput+1),hl
    ld (bankSerialOutput+1),hl
    ld (bankWindowOutput+1),hl
    pop hl
    inc hl
    ld a,(hl)
    ld (matrixKind),a
    cp 3
    jp c,matrixRegularKind
    sub 3
    add a,a
matrixRegularKind:
    ld (bankImage+0180h),a
    inc hl
    ld a,(hl)
    ld (matrixFlags),a
    and 80
    jp z,matrixBankKindReady
    ld a,3
    ld (bankImage+0180h),a
    ld a,(matrixFlags)
    and 64
    jp z,matrixBankKindReady
    ld a,4
    ld (bankImage+0180h),a
matrixBankKindReady:
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
    call matrixClearText
    ld a,(matrixBank)
    or a
    jp nz,matrixRunBank
    ld a,(matrixKind)
    or a
    jp z,matrixRunScalar
    cp 3
    jp z,matrixRunScalar
    dec a
    jp z,matrixRunQuad
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
matrixMeasured:
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
matrixDirectionDone:
    ld a,(matrixBank)
    inc a
    ld (matrixBank),a
    cp 2
    jp nz,matrixDirection
    call restoreOriginal
    call cacheDiscard
    call matrixSaveReport
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
    ld hl,(matrixCase)
    ld de,4
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    call print_time_ms
    ld hl,slashText
    call puts
    ld hl,(matrixCase)
    ld de,6
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    call print_time_ms
    ld hl,newlineText
    call puts
    ret
matrixPrinted:
    ld a,(matrixSummaryY)
    ld l,a
    ld h,0
    call bios_setCursorPos
    ld hl,(bitmapResult)
    call puts
    ld hl,(reportMessage)
    call puts
    jp bench_finish

checkBankFrame:
    ld a,(matrixKind)
    cp 3
    jp nc,checkAligned8Frame
    ld hl,09000h
    ld de,06800h
    ld b,36
checkBankColumn:
    ld c,FONT_ROWS*8
checkBankByte:
    ld a,(de)
    cp (hl)
    jp nz,bankMismatch
    inc hl
    inc de
    dec c
    jp nz,checkBankByte
    ld a,l
    add a,256-FONT_ROWS*8
    ld l,a
    jp nc,checkBankNoCarry
    inc h
checkBankNoCarry:
    dec b
    jp nz,checkBankColumn
    ret
; Эталон хранит только 192 занятые строки каждого из 36 столбцов.
; 6912 байт по 6800h оставляют место для резерва DOS и корпуса.
saveReference:
    ld hl,09000h
    ld de,06800h
    ld b,36
saveReferenceColumn:
    ld c,FONT_ROWS*8
saveReferenceByte:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec c
    jp nz,saveReferenceByte
    ld a,l
    add a,256-FONT_ROWS*8
    ld l,a
    jp nc,saveReferenceNoCarry
    inc h
saveReferenceNoCarry:
    dec b
    jp nz,saveReferenceColumn
    ret
bankMismatch:
    ld hl,failText
    ld (bitmapResult),hl
    ret

renderScreen:
    call reset_timing
    ld a,FONT_ROWS
    ld (mainRows),a
    ld de,textCorpus
    ld hl,00007h
row:
    ld (bios_vars.cursorY),hl
    push hl
    ld b,FONT_COLUMNS
glyph:
    ld a,(de)
    ld c,a
output:
    call bios_printChar
    inc de
    dec b
    jp nz,glyph
    call timer_sample
    pop hl
    ld a,l
    add a,8
    ld l,a
    ld a,(mainRows)
    dec a
    ld (mainRows),a
    jp nz,row
    ret
renderQuadScreen:
    call reset_timing
    ld a,FONT_ROWS
    ld (mainRows),a
    ld de,textCorpus
    ld hl,00007h
quadScreenRow:
    ld (bios_vars.cursorY),hl
    push hl
    ld b,FONT_COLUMNS/4
quadScreenGroup:
    ld h,d
    ld l,e
    ld c,4
quadOutput:
    call quadRender
    jp c,quadScreenError
    inc de
    inc de
    inc de
    inc de
    dec b
    jp nz,quadScreenGroup
    call timer_sample
    pop hl
    ld a,l
    add a,8
    ld l,a
    ld a,(mainRows)
    dec a
    ld (mainRows),a
    jp nz,quadScreenRow
    ret
quadScreenError:
    pop hl
    ld hl,failText
    ld (bitmapResult),hl
    ret
renderSerialScreen:
    call reset_timing
    ld a,FONT_ROWS
    ld (mainRows),a
    ld de,textCorpus
    ld hl,00007h
serialScreenRow:
    ld (bios_vars.cursorY),hl
    push hl
    ld h,d
    ld l,e
    ld c,48
serialOutput:
    call serialBIOS
    jp c,quadScreenError
    ld hl,48
    add hl,de
    ex de,hl
    call timer_sample
    pop hl
    ld a,l
    add a,8
    ld l,a
    ld a,(mainRows)
    dec a
    ld (mainRows),a
    jp nz,serialScreenRow
    ret
corpusNoMemory:
    ld hl,corpusMemoryText
    call puts
    jp bench_finish
corpusFailed:
    ld hl,corpusErrorText
    call puts
    jp bench_finish
title: DB 'FONTBEN README 48x24, ms: main / DOS',13,10,0
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
    MACRO CASE core,kind,flags,mainTime,bankTime,mainMiss,bankMiss,label
    DW core
    DB kind,flags
    DW mainTime,bankTime,mainMiss,bankMiss,label
    ENDM
matrixCases:
    CASE bios_printChar,0,0,biosTicks,bankBiosTicks,0,0,labelBIOS
    CASE bios_printChar,0,4,optTicks,bankOptTicks,0,0,labelOpt
    CASE serialBIOS,2,0,serialBiosTicks,bankSerialBiosTicks,0,0,labelSerialBIOS
    CASE serialBIOS,2,4,serialOptTicks,bankSerialOptTicks,0,0,labelSerialOpt
    CASE 02000h,0,0,shiftTicks,bankShiftTicks,0,0,labelFull
    CASE serialFull,2,0,serialFullTicks,bankSerialFullTicks,0,0,labelSerialFull
    CASE cacheRender,0,3,coldTicks,bankColdTicks,coldMisses,bankColdMisses,labelCold
    CASE cacheRender,0,2,warmTicks,bankWarmTicks,warmMisses,bankWarmMisses,labelWarm
    CASE serialCache,2,3,serialColdTicks,bankSerialColdTicks,serialColdMisses,bankSerialColdMisses,labelSerialCold
    CASE serialCache,2,2,serialWarmTicks,bankSerialWarmTicks,serialWarmMisses,bankSerialWarmMisses,labelSerialWarm
    CASE quadFullRender,1,0,quadFullTicks,bankQuadFullTicks,0,0,labelQuadFull
    CASE quadRender,1,3,quadColdTicks,bankQuadColdTicks,quadColdMisses,bankQuadColdMisses,labelQuadCold
    CASE quadRender,1,2,quadWarmTicks,bankQuadWarmTicks,quadWarmMisses,bankQuadWarmMisses,labelQuadWarm
    CASE quadDynamicRender,1,0,quadDynamicTicks,bankQuadDynamicTicks,0,0,labelRaw
    CASE quadHalfRender,1,8,quadHalfTicks,bankQuadHalfTicks,0,0,labelHalf
    CASE serialBIOS,2,16,windowBiosMain,windowBiosTicks,0,0,labelWindowBIOS
    CASE serialBIOS,2,20,windowOptMain,windowOptTicks,0,0,labelWindowOpt
    CASE serialFull,2,16,windowFullMain,windowFullTicks,0,0,labelWindowFull
    CASE quadFullRender,2,16,windowQuadMain,windowQuadTicks,0,0,labelWindowQuad
    CASE serialCache,2,19,windowColdMain,windowColdTicks,windowColdMainMiss,windowColdMisses,labelWindowCold
    CASE serialCache,2,18,windowWarmMain,windowWarmTicks,windowWarmMainMiss,windowWarmMisses,labelWindowWarm
    CASE serialBIOS,2,32,bulkBiosMain,bulkBiosTicks,0,0,labelBulkBIOS
    CASE serialBIOS,2,36,bulkOptMain,bulkOptTicks,0,0,labelBulkOpt
    CASE serialFull,2,32,bulkFullMain,bulkFullTicks,0,0,labelBulkFull
    CASE quadFullRender,2,32,bulkQuadMain,bulkQuadTicks,0,0,labelBulkQuad
    CASE serialCache,2,35,bulkColdMain,bulkColdTicks,bulkColdMainMiss,bulkColdMisses,labelBulkCold
    CASE serialCache,2,34,bulkWarmMain,bulkWarmTicks,bulkWarmMainMiss,bulkWarmMisses,labelBulkWarm
    CASE quadPackedRender,1,128,PackedMain,PackedTicks,0,0,labelPacked
    CASE quadRender,2,19,Win20ColdMain,Win20ColdTicks,Win20ColdMainMiss,Win20ColdMisses,labelWin20Cold
    CASE quadRender,2,18,Win20WarmMain,Win20WarmTicks,Win20WarmMainMiss,Win20WarmMisses,labelWin20Warm
    CASE quadDynamicRender,2,16,Win20RawMain,Win20RawTicks,0,0,labelWin20Raw
    CASE quadHalfRender,2,24,Win20HalfMain,Win20HalfTicks,0,0,labelWin20Half
    CASE quadPackedRender,2,144,Win20PackedMain,Win20PackedTicks,0,0,labelWin20Packed
    CASE quadRender,2,35,Far1152ColdMain,Far1152ColdTicks,Far1152ColdMainMiss,Far1152ColdMisses,labelFar1152Cold
    CASE quadRender,2,34,Far1152WarmMain,Far1152WarmTicks,Far1152WarmMainMiss,Far1152WarmMisses,labelFar1152Warm
    CASE quadDynamicRender,2,32,Far1152RawMain,Far1152RawTicks,0,0,labelFar1152Raw
    CASE quadHalfRender,2,40,Far1152HalfMain,Far1152HalfTicks,0,0,labelFar1152Half
    CASE quadPackedRender,2,160,Far1152PackedMain,Far1152PackedTicks,0,0,labelFar1152Packed
    CASE quadRender,2,67,Far48ColdMain,Far48ColdTicks,Far48ColdMainMiss,Far48ColdMisses,labelFar48Cold
    CASE quadRender,2,66,Far48WarmMain,Far48WarmTicks,Far48WarmMainMiss,Far48WarmMisses,labelFar48Warm
    CASE quadDynamicRender,2,64,Far48RawMain,Far48RawTicks,0,0,labelFar48Raw
    CASE quadHalfRender,2,72,Far48HalfMain,Far48HalfTicks,0,0,labelFar48Half
    CASE quadPackedRender,2,192,Far48PackedMain,Far48PackedTicks,0,0,labelFar48Packed
    CASE serialBIOS,2,64,Far48BiosMain,Far48BiosTicks,0,0,labelFar48Bios
    CASE serialBIOS,2,68,Far48OptMain,Far48OptTicks,0,0,labelFar48Opt
    CASE serialFull,2,64,Far48FullMain,Far48FullTicks,0,0,labelFar48Full
    CASE quadFullRender,2,64,Far48QuadMain,Far48QuadTicks,0,0,labelFar48Quad
    CASE serialCache,2,67,Far48ColdSerialMain,Far48ColdSerialTicks,Far48ColdSerialMainMiss,Far48ColdSerialMisses,labelFar48ColdSerial
    CASE serialCache,2,66,Far48WarmSerialMain,Far48WarmSerialTicks,Far48WarmSerialMainMiss,Far48WarmSerialMisses,labelFar48WarmSerial
    CASE aligned8Scalar,3,0,font8Ticks,bankFont8Ticks,0,0,labelFont8
    CASE aligned8Batch,4,0,font8SerialTicks,bankFont8SerialTicks,0,0,labelFont8Serial
    CASE aligned8Batch,4,16,window8Main,window8Ticks,0,0,labelWindow8
    CASE aligned8Batch,4,64,Far48Font8Main,Far48Font8Ticks,0,0,labelFar48Font8
    CASE aligned8Batch,4,32,bulk8Main,bulk8Ticks,0,0,labelBulk8
matrixCasesEnd:
MATRIX_CASE_COUNT = (matrixCasesEnd-matrixCases)/14
MATRIX_STAGE_COUNT = MATRIX_CASE_COUNT*2
    ASSERT MATRIX_STAGE_COUNT < 256
    DW 0
labelBIOS: DB 'BIOS char:   ',0
labelFont8: DB '8x8 char:    ',0
font8Ticks: DD 0
bankFont8Ticks: DD 0
labelFont8Serial: DB '8x8 serial:  ',0
font8SerialTicks: DD 0
bankFont8SerialTicks: DD 0
labelWindow8: DB 'Win20 8x8:   ',0
window8Main: DD 0
window8Ticks: DD 0
labelFar48Font8: DB 'Far48 8x8:   ',0
Far48Font8Main: DD 0
Far48Font8Ticks: DD 0
labelBulk8: DB 'Far1152 8x8: ',0
bulk8Main: DD 0
bulk8Ticks: DD 0
labelWindowOpt: DB 'Win20 opt:   ',0
windowOptMain: DD 0
windowOptTicks: DD 0
labelBulkOpt: DB 'Far1152 opt: ',0
bulkOptMain: DD 0
bulkOptTicks: DD 0
labelFar48Opt: DB 'Far48 opt:   ',0
Far48OptMain: DD 0
Far48OptTicks: DD 0
labelOpt: DB 'Opt char:    ',0
labelSerialBIOS: DB 'BIOS serial: ',0
labelSerialOpt: DB 'Opt serial:  ',0
labelFull: DB 'Full char:   ',0
labelSerialFull: DB 'Full serial: ',0
labelCold: DB 'Cache cold:  ',0
labelWarm: DB 'Cache warm:  ',0
labelSerialCold: DB 'Serial cold: ',0
labelSerialWarm: DB 'Serial warm: ',0
labelQuadFull: DB 'Quad full:   ',0
labelQuadCold: DB 'Quad cold:   ',0
labelQuadWarm: DB 'Quad warm:   ',0
labelRaw: DB 'Quad raw:    ',0
labelHalf: DB 'Quad rot4:   ',0
labelWindowBIOS: DB 'Win20 BIOS:  ',0
labelWindowFull: DB 'Win20 full:  ',0
labelWindowQuad: DB 'Win20 quad:  ',0
labelWindowCold: DB 'Win20 cold:  ',0
labelWindowWarm: DB 'Win20 warm:  ',0
labelBulkBIOS: DB 'Far1152 BIOS:',0
labelBulkFull: DB 'Far1152 full:',0
labelBulkQuad: DB 'Far1152 quad:',0
labelBulkCold: DB 'Far1152 cold:',0
labelBulkWarm: DB 'Far1152 warm:',0
biosTicks: DD 0
bankBiosTicks: DD 0
optTicks: DD 0
bankOptTicks: DD 0
serialBiosTicks: DD 0
bankSerialBiosTicks: DD 0
serialOptTicks: DD 0
bankSerialOptTicks: DD 0
shiftTicks: DD 0
bankShiftTicks: DD 0
serialFullTicks: DD 0
bankSerialFullTicks: DD 0
coldTicks: DD 0
bankColdTicks: DD 0
warmTicks: DD 0
bankWarmTicks: DD 0
serialColdTicks: DD 0
bankSerialColdTicks: DD 0
serialWarmTicks: DD 0
bankSerialWarmTicks: DD 0
quadFullTicks: DD 0
bankQuadFullTicks: DD 0
quadColdTicks: DD 0
bankQuadColdTicks: DD 0
quadWarmTicks: DD 0
bankQuadWarmTicks: DD 0
quadDynamicTicks: DD 0
bankQuadDynamicTicks: DD 0
quadHalfTicks: DD 0
bankQuadHalfTicks: DD 0
coldMisses: DW 0
warmMisses: DW 0
bankColdMisses: DW 0
bankWarmMisses: DW 0
serialColdMisses: DW 0
serialWarmMisses: DW 0
bankSerialColdMisses: DW 0
bankSerialWarmMisses: DW 0
quadColdMisses: DW 0
quadWarmMisses: DW 0
bankQuadColdMisses: DW 0
bankQuadWarmMisses: DW 0
windowBiosMain: DD 0
windowBiosTicks: DD 0
windowFullMain: DD 0
windowFullTicks: DD 0
windowColdMain: DD 0
windowColdTicks: DD 0
windowWarmMain: DD 0
windowWarmTicks: DD 0
bulkBiosMain: DD 0
bulkBiosTicks: DD 0
bulkFullMain: DD 0
bulkFullTicks: DD 0
bulkColdMain: DD 0
bulkColdTicks: DD 0
bulkWarmMain: DD 0
bulkWarmTicks: DD 0
windowColdMainMiss: DW 0
windowWarmMainMiss: DW 0
windowColdMisses: DW 0
windowWarmMisses: DW 0
bulkColdMainMiss: DW 0
bulkWarmMainMiss: DW 0
bulkColdMisses: DW 0
bulkWarmMisses: DW 0
windowQuadMain: DD 0
windowQuadTicks: DD 0
bulkQuadMain: DD 0
bulkQuadTicks: DD 0

labelPacked: DB 'Quad KOI4072:',0
PackedMain: DD 0
PackedTicks: DD 0
labelWin20Cold: DB 'Win20 q cold:',0
Win20ColdMain: DD 0
Win20ColdTicks: DD 0
Win20ColdMainMiss: DW 0
Win20ColdMisses: DW 0
labelWin20Warm: DB 'Win20 q warm:',0
Win20WarmMain: DD 0
Win20WarmTicks: DD 0
Win20WarmMainMiss: DW 0
Win20WarmMisses: DW 0
labelWin20Raw: DB 'Win20 q raw:',0
Win20RawMain: DD 0
Win20RawTicks: DD 0
labelWin20Half: DB 'Win20 q rot4:',0
Win20HalfMain: DD 0
Win20HalfTicks: DD 0
labelWin20Packed: DB 'Win20 KOI4072:',0
Win20PackedMain: DD 0
Win20PackedTicks: DD 0
labelFar1152Cold: DB 'Far1152 q cold:',0
Far1152ColdMain: DD 0
Far1152ColdTicks: DD 0
Far1152ColdMainMiss: DW 0
Far1152ColdMisses: DW 0
labelFar1152Warm: DB 'Far1152 q warm:',0
Far1152WarmMain: DD 0
Far1152WarmTicks: DD 0
Far1152WarmMainMiss: DW 0
Far1152WarmMisses: DW 0
labelFar1152Raw: DB 'Far1152 q raw:',0
Far1152RawMain: DD 0
Far1152RawTicks: DD 0
labelFar1152Half: DB 'Far1152 q rot4:',0
Far1152HalfMain: DD 0
Far1152HalfTicks: DD 0
labelFar1152Packed: DB 'Far1152 KOI4072:',0
Far1152PackedMain: DD 0
Far1152PackedTicks: DD 0
labelFar48Cold: DB 'Far48 q cold:',0
Far48ColdMain: DD 0
Far48ColdTicks: DD 0
Far48ColdMainMiss: DW 0
Far48ColdMisses: DW 0
labelFar48Warm: DB 'Far48 q warm:',0
Far48WarmMain: DD 0
Far48WarmTicks: DD 0
Far48WarmMainMiss: DW 0
Far48WarmMisses: DW 0
labelFar48Raw: DB 'Far48 q raw:',0
Far48RawMain: DD 0
Far48RawTicks: DD 0
labelFar48Half: DB 'Far48 q rot4:',0
Far48HalfMain: DD 0
Far48HalfTicks: DD 0
labelFar48Packed: DB 'Far48 KOI4072:',0
Far48PackedMain: DD 0
Far48PackedTicks: DD 0
labelFar48Bios: DB 'Far48 BIOS:',0
Far48BiosMain: DD 0
Far48BiosTicks: DD 0
labelFar48Full: DB 'Far48 full:',0
Far48FullMain: DD 0
Far48FullTicks: DD 0
labelFar48Quad: DB 'Far48 q full:',0
Far48QuadMain: DD 0
Far48QuadTicks: DD 0
labelFar48ColdSerial: DB 'Far48 s cold:',0
Far48ColdSerialMain: DD 0
Far48ColdSerialTicks: DD 0
Far48ColdSerialMainMiss: DW 0
Far48ColdSerialMisses: DW 0
labelFar48WarmSerial: DB 'Far48 s warm:',0
Far48WarmSerialMain: DD 0
Far48WarmSerialTicks: DD 0
Far48WarmSerialMainMiss: DW 0
Far48WarmSerialMisses: DW 0
