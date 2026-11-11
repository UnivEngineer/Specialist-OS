    MACRO CASE core,kind,flags,mainTime,bankTime,mainMiss,bankMiss,label
    DW core
    DB kind,flags
    DW mainTime,bankTime,mainMiss,bankMiss,label
    ENDM
matrixCases:
    CASE monitor.scalar,0,0,biosTicks,bankBiosTicks,0,0,labelBIOS
    CASE monitor.scalar,0,4,optTicks,bankOptTicks,0,0,labelOpt
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
    CASE font4Scalar,5,0,f4RawCharMain,f4RawCharDOS,0,0,label4RawChar
    CASE font4Batch,6,0,f4RawSerialMain,f4RawSerialDOS,0,0,label4RawSerial
    CASE font4Batch,6,16,f4RawWin20Main,f4RawWin20DOS,0,0,label4RawWin20
    CASE font4Batch,6,64,f4RawFar48Main,f4RawFar48DOS,0,0,label4RawFar48
    CASE font4Batch,6,32,f4RawFar1152Main,f4RawFar1152DOS,0,0,label4RawFar1152
    CASE font4Scalar,7,0,f4PackedCharMain,f4PackedCharDOS,0,0,label4PackedChar
    CASE font4Batch,8,0,f4PackedSerialMain,f4PackedSerialDOS,0,0,label4PackedSerial
    CASE font4Batch,8,16,f4PackedWin20Main,f4PackedWin20DOS,0,0,label4PackedWin20
    CASE font4Batch,8,64,f4PackedFar48Main,f4PackedFar48DOS,0,0,label4PackedFar48
    CASE font4Batch,8,32,f4PackedFar1152Main,f4PackedFar1152DOS,0,0,label4PackedFar1152
    CASE font4Scalar,9,0,f4RepeatCharMain,f4RepeatCharDOS,0,0,label4RepeatChar
    CASE font4Batch,10,0,f4RepeatSerialMain,f4RepeatSerialDOS,0,0,label4RepeatSerial
    CASE font4Batch,10,16,f4RepeatWin20Main,f4RepeatWin20DOS,0,0,label4RepeatWin20
    CASE font4Batch,10,64,f4RepeatFar48Main,f4RepeatFar48DOS,0,0,label4RepeatFar48
    CASE font4Batch,10,32,f4RepeatFar1152Main,f4RepeatFar1152DOS,0,0,label4RepeatFar1152
    CASE kernelChar,0,0,apiCharMain,apiCharBank,0,0,labelApiChar
    CASE kernelReal,0,0,apiRealMain,apiRealBank,0,0,labelApiReal
    CASE kernelString,2,16,apiStringMain,apiStringBank,0,0,labelApiString
    CASE kernelString,2,64,apiFar48Main,apiFar48Bank,0,0,labelApiFar48
    CASE kernelString,2,32,apiBulkMain,apiBulkBank,0,0,labelApiBulk
    CASE pagedRaw,2,16,PageRawLineMain,PageRawLineBank,0,0,labelPageRawLine
    CASE pagedRaw,2,64,PageRawFar48Main,PageRawFar48Bank,0,0,labelPageRawFar48
    CASE pagedRaw,2,32,PageRawFar1152Main,PageRawFar1152Bank,0,0,labelPageRawFar1152
    CASE pagedPre,2,16,PagePreLineMain,PagePreLineBank,0,0,labelPagePreLine
    CASE pagedPre,2,64,PagePreFar48Main,PagePreFar48Bank,0,0,labelPagePreFar48
    CASE pagedPre,2,32,PagePreFar1152Main,PagePreFar1152Bank,0,0,labelPagePreFar1152
    CASE paged8,4,16,Page8LineMain,Page8LineBank,0,0,labelPage8Line
    CASE paged8,4,64,Page8Far48Main,Page8Far48Bank,0,0,labelPage8Far48
    CASE paged8,4,32,Page8Far1152Main,Page8Far1152Bank,0,0,labelPage8Far1152
    CASE kernelRealString,2,16,ApiRealWin20Main,ApiRealWin20Bank,0,0,labelApiRealWin20
    CASE kernelRealString,2,64,ApiRealFar48Main,ApiRealFar48Bank,0,0,labelApiRealFar48
    CASE kernelRealString,2,32,ApiRealFar1152Main,ApiRealFar1152Bank,0,0,labelApiRealFar1152
matrixCasesEnd:
MATRIX_CASE_COUNT = (matrixCasesEnd-matrixCases)/14
MATRIX_STAGE_COUNT = MATRIX_CASE_COUNT*2
    ASSERT MATRIX_STAGE_COUNT < 256
    DW 0
labelBIOS: DB 'Monitor char:',0
labelFont8: DB '8x8 char:    ',0
font8Ticks: DD 0
bankFont8Ticks: DD 0
labelFont8Serial: DB '8x8 serial:  ',0
font8SerialTicks: DD 0
bankFont8SerialTicks: DD 0
labelWindow8: DB 'Win20  8x8:   ',0
window8Main: DD 0
window8Ticks: DD 0
labelFar48Font8: DB 'Far48 8x8:   ',0
Far48Font8Main: DD 0
Far48Font8Ticks: DD 0
labelBulk8: DB 'Far1152 8x8: ',0
bulk8Main: DD 0
bulk8Ticks: DD 0
labelWindowOpt: DB 'Win20  opt:   ',0
windowOptMain: DD 0
windowOptTicks: DD 0
labelBulkOpt: DB 'Far1152 opt: ',0
bulkOptMain: DD 0
bulkOptTicks: DD 0
labelFar48Opt: DB 'Far48 opt:   ',0
Far48OptMain: DD 0
Far48OptTicks: DD 0
labelOpt: DB 'Opt char:    ',0
labelSerialBIOS: DB 'Monitor line:',0
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
labelWindowBIOS: DB 'Win20  Mon:   ',0
labelWindowFull: DB 'Win20  full:  ',0
labelWindowQuad: DB 'Win20  quad:  ',0
labelWindowCold: DB 'Win20  cold:  ',0
labelWindowWarm: DB 'Win20  warm:  ',0
labelBulkBIOS: DB 'Far1152 Mon: ',0
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
labelWin20Cold: DB 'Win20  q cold:',0
Win20ColdMain: DD 0
Win20ColdTicks: DD 0
Win20ColdMainMiss: DW 0
Win20ColdMisses: DW 0
labelWin20Warm: DB 'Win20  q warm:',0
Win20WarmMain: DD 0
Win20WarmTicks: DD 0
Win20WarmMainMiss: DW 0
Win20WarmMisses: DW 0
labelWin20Raw: DB 'Win20  q raw:',0
Win20RawMain: DD 0
Win20RawTicks: DD 0
labelWin20Half: DB 'Win20  q rot4:',0
Win20HalfMain: DD 0
Win20HalfTicks: DD 0
labelWin20Packed: DB 'Win20  KOI4072:',0
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
labelFar48Bios: DB 'Far48 Mon: ',0
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
label4RawChar: DB '4raw char:',0
f4RawCharMain: DD 0
f4RawCharDOS: DD 0
label4RawSerial: DB '4raw serial:',0
f4RawSerialMain: DD 0
f4RawSerialDOS: DD 0
label4RawWin20: DB '4raw Win20 :',0
f4RawWin20Main: DD 0
f4RawWin20DOS: DD 0
label4RawFar48: DB '4raw Far48:',0
f4RawFar48Main: DD 0
f4RawFar48DOS: DD 0
label4RawFar1152: DB '4raw Far1152:',0
f4RawFar1152Main: DD 0
f4RawFar1152DOS: DD 0
label4PackedChar: DB '4pack char:',0
f4PackedCharMain: DD 0
f4PackedCharDOS: DD 0
label4PackedSerial: DB '4pack serial:',0
f4PackedSerialMain: DD 0
f4PackedSerialDOS: DD 0
label4PackedWin20: DB '4pack Win20 :',0
f4PackedWin20Main: DD 0
f4PackedWin20DOS: DD 0
label4PackedFar48: DB '4pack Far48:',0
f4PackedFar48Main: DD 0
f4PackedFar48DOS: DD 0
label4PackedFar1152: DB '4pack Far1152:',0
f4PackedFar1152Main: DD 0
f4PackedFar1152DOS: DD 0
label4RepeatChar: DB '4dup char:',0
f4RepeatCharMain: DD 0
f4RepeatCharDOS: DD 0
label4RepeatSerial: DB '4dup serial:',0
f4RepeatSerialMain: DD 0
f4RepeatSerialDOS: DD 0
label4RepeatWin20: DB '4dup Win20 :',0
f4RepeatWin20Main: DD 0
f4RepeatWin20DOS: DD 0
label4RepeatFar48: DB '4dup Far48:',0
f4RepeatFar48Main: DD 0
f4RepeatFar48DOS: DD 0
label4RepeatFar1152: DB '4dup Far1152:',0
f4RepeatFar1152Main: DD 0
f4RepeatFar1152DOS: DD 0
labelPageRawLine: DB 'PageRaw Win20 :',0
PageRawLineMain: DD 0
PageRawLineBank: DD 0
labelPageRawFar48: DB 'PageRaw Far48:',0
PageRawFar48Main: DD 0
PageRawFar48Bank: DD 0
labelPageRawFar1152: DB 'PageRaw Far1152:',0
PageRawFar1152Main: DD 0
PageRawFar1152Bank: DD 0
labelPagePreLine: DB 'PagePre Win20 :',0
PagePreLineMain: DD 0
PagePreLineBank: DD 0
labelPagePreFar48: DB 'PagePre Far48:',0
PagePreFar48Main: DD 0
PagePreFar48Bank: DD 0
labelPagePreFar1152: DB 'PagePre Far1152:',0
PagePreFar1152Main: DD 0
PagePreFar1152Bank: DD 0
labelPage8Line: DB 'Page8 Win20 :',0
Page8LineMain: DD 0
Page8LineBank: DD 0
labelPage8Far48: DB 'Page8 Far48:',0
Page8Far48Main: DD 0
Page8Far48Bank: DD 0
labelPage8Far1152: DB 'Page8 Far1152:',0
Page8Far1152Main: DD 0
Page8Far1152Bank: DD 0
labelApiRealWin20: DB 'API real Win20 :',0
ApiRealWin20Main: DD 0
ApiRealWin20Bank: DD 0
labelApiRealFar48: DB 'API real Far48:',0
ApiRealFar48Main: DD 0
ApiRealFar48Bank: DD 0
labelApiRealFar1152: DB 'API real Far1152:',0
ApiRealFar1152Main: DD 0
ApiRealFar1152Bank: DD 0
