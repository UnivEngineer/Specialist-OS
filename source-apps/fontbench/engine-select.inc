; A MAIN renderer DLL owns [2000h,end); the controller and five frontends
; stay below 2000h. Only initialization uses MODULE_CALL, never timed pixels.
; Native BIOS keeps its installed persistent resources; only the benchmark
; scratch/report uses 3000h. Standalone scratch/report is above its DLL, 7C00h.
engineOpen:
    call engineRecord
    ld de,7
    add hl,de
    ld a,(hl)
    ld (engineHistoricalMode),a
    ld a,(engineId)
    or a
    jp nz,engineStandalone
    ; Installed native DLL owns its persistent resources. FontBen neither
    ; rebuilds nor disables it. Test exactly the state selected with FONT.
    ld hl,03000h
    ld (gridControl+8),hl
    ld hl,(FONT_ENGINE_HEADER+12)
    ld a,h
    or l
    ld a,0
    jp z,nativePreReady
    inc a
nativePreReady:
    ld b,a
    ld a,(gridPre)
    cp b
    ret z
    ld a,DOS_ERR_ARGUMENT
    scf
    ret
engineStandalone:
    ; Never place a standalone DLL over a resident prerender reservation.
    call mx_getMemTop
    ld de,084FFh
    call mon2_cmp_hl_de
    ret c
    ld hl,07C00h
    ld (gridControl+8),hl
    ld b,0
    ld hl,02000h
    ld de,05C00h
    ld a,MEM_OP_ARENA
    MXOS_CALL_REGS MXOS_DOS_MEMORY
    ret c
engineLoad:
    call engineRecord
    inc hl
    inc hl
    ld a,(hl)
    ld (engineRegs+7),a
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld de,engineBasename
engineCopyBasename:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    or a
    jp nz,engineCopyBasename
    ld hl,enginePath
    MXOS_CALL_REGS MXOS_DOS_MODULE_LOAD
    ret c
    ld (engineHandle),hl
    ld a,(gridPre)
    ld (engineRegs+5),a
    ld de,engineRegs
    MXOS_CALL_REGS MXOS_DOS_MODULE_CALL
    ret c
    ld a,(engineRegs+6)
    rra
    ret c
    ld a,(engineHistoricalMode)
    and 80h
    ld hl,(engineRegs)
    jp nz,directOpen
    ld a,(hl)
    ld (engineWidth),a
    inc hl
    ld a,(hl)
    cpl
    inc a
    ld (hookRoundMask+1),a
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld (engineScalar+1),hl
    ex de,hl
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld (engineBatch+1),hl
    ex de,hl
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld (engineResetStage+1),hl
    ex de,hl
    ld a,(engineHistoricalMode)
    or a
    jp z,engineHistoricalReady
    inc hl ; optional descriptor word at +8, after reset callback
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld (historicalGlyph+1),hl
engineHistoricalReady:
    ld a,(engineWidth)
    ld hl,bounds6
    cp 6
    jp z,engineBoundsReady
    ld hl,bounds4
    cp 4
    jp z,engineBoundsReady
    ld hl,bounds8
engineBoundsReady:
    ld (hookBounds+1),hl
    or a
    ret
; Every stage restores the entire ABI before progress/UI/disk calls. The
; original addresses are saved once, so repeated stages cannot save hooks.
engineHook:
    ld a,(engineId)
    or a
    ret z
    ld a,(engineHistoricalMode)
    and 80h
    call z,engineResetStage
    call engineInstall
    ld a,1
    ld (engineHooked),a
    ; Warm means a complete untimed pass over this same corpus, not merely
    ; reusing a cache left by a different caller. Clear pixels after priming.
    ld a,(engineId)
    cp 7
    ret nz
    ld a,(gridPre)
    or a
    ret z
    ld hl,0007h
    ld (mon2_vars.cursorY),hl
    ld hl,gridText
    ld bc,GRID_TEXT_BYTES
    call hookLim
    jp gridClear
engineUnhook:
    ld a,(engineHooked)
    or a
    ret z
    call engineRestore
    xor a
    ld (engineHooked),a
    ret
engineClose:
    call engineUnhook
    ld hl,(engineHandle)
    ld a,h
    or l
    jp z,engineFree
    xor a
    MXOS_CALL_REGS MXOS_DOS_MODULE_UNLOAD
    ld hl,0
    ld (engineHandle),hl
engineFree:
    ld hl,(engineAllocation)
    ld a,h
    or l
    ret z
    ld b,0
    ld a,MEM_OP_FREE
    MXOS_CALL_REGS MXOS_DOS_MEMORY
    ld hl,0
    ld (engineAllocation),hl
    ret
; Eight-byte record: name, variant, DLL path, selected, width, historical mode.
engineRecord:
    ld a,(engineId)
    ld l,a
    ld h,0
    add hl,hl
    add hl,hl
    add hl,hl
    ld de,engineRecords
    add hl,de
    ret
engineName:
    call engineRecord
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ret
engineSelected:
    call engineRecord
    ld de,5
    add hl,de
    ld a,(hl)
    ret
engineSelectShared:
    xor a
    call engineSelectAll
    ld hl,sharedEngineRecords+5
    ld b,(windowEngineRecord-sharedEngineRecords)/8
    ld a,1
    call engineSelectLoop
    ld hl,experimentalEngineRecords+5
    ld b,(engineRecordsEnd-experimentalEngineRecords)/8
    jp engineSelectLoop
engineSelectAll:
    ld hl,engineRecords+5
    ld b,ENGINE_COUNT
engineSelectLoop:
    ld (hl),a
    ld de,8
    add hl,de
    dec b
    jp nz,engineSelectLoop
    ret
engineId:           DB 0
engineWidth:        DB 6
engineHooked:       DB 0
engineHandle:       DW 0
engineAllocation:   DW 0
engineRegs:         DS 8,0
engineFirstArg:     DB 0
enginePreFirst:     DB 0
enginePreEnd:       DB 2
engineFileAppend:   DB 0
    MACRO ENGINE name, variant, path, selected, width
    DW name
    DB variant
    DW path
    DB selected,width,0
    ENDM
; The final record byte selects an authentic historical ASCIZ loop:
; 1=MXOS1, 2=Monitor-2, 3=RAMFOS. Other engines retain the shared frontend.
    MACRO HISTORICAL_ENGINE name, variant, path, mode
    DW name
    DB variant
    DW path
    DB 0,6,mode
    ENDM
engineRecords:
    ENGINE nameBios,0,0,1,6
    HISTORICAL_ENGINE nameMonitor,0,path6,1
    ENGINE nameOpt,1,path6,0,6
    ENGINE nameFull,2,path6,0,6
    ENGINE nameCache,3,path6,0,6
    ENGINE nameQuad,4,path6,0,6
    ENGINE nameCold,5,path6,0,6
    ENGINE nameWarm,6,path6,0,6
    ENGINE nameRotate,7,path6,0,6
    ENGINE nameHalf,8,path6,0,6
    ENGINE namePacked,9,path6,0,6
    ENGINE namePaged,0,pathP,0,6
    ENGINE nameEight,1,pathP,0,8
    ENGINE nameFour,0,path4,0,4
    ENGINE nameFourPack,1,path4,0,4
    ENGINE nameFourDup,2,path4,0,4
    ENGINE nameRemap,0,pathM,0,6
    ENGINE nameBlock,0,pathB,0,6
    HISTORICAL_ENGINE nameMon2,0,pathS,2
    HISTORICAL_ENGINE nameRamfos,1,pathS,3
    ENGINE nameLinear,2,pathS,0,6
    ENGINE nameFullFast,3,pathS,0,6
    ENGINE nameColPre,4,pathS,0,6
    ENGINE nameTriple,0,pathQ,0,6
sharedEngineRecords:
rotateQuadsRecord:
    DW nameFontRot
    DB 0
    DW pathFontRot
    DB 0,6,80h
packedQuadsRecord:
    DW nameFontPack
    DB 1
    DW pathFontPack
    DB 0,6,80h
fullQuadsRecord:
    DW nameFontFull
    DB 1
    DW pathFontFull
    DB 0,6,80h
    DW nameFontKoi
    DB 0
    DW pathFontKoi
    DB 0,6,80h
    DW nameFont8
    DB 0
    DW pathFont8
    DB 0,8,80h
    DW nameFont4
    DB 0
    DW pathFont4
    DB 0,4,80h
    DW nameFont4Fast
    DB 1
    DW pathFont4Fast
    DB 0,4,80h
    DW nameRot6Col4
    DB 0
    DW pathRot6Col4
    DB 0,6,80h
windowEngineRecord:
    DW nameWindow
    DB 0
    DW pathWindow
    DB 0,8,4
experimentalEngineRecords:
    DW nameRotRows
    DB 0
    DW pathRotRows
    DB 0,6,80h
    DW nameRot8Quad
    DB 0
    DW pathRot8Quad
    DB 0,8,80h
    DW nameRot8Stream
    DB 0
    DW pathRot8Stream
    DB 0,8,80h
    MACRO DIRECT_ENGINE name, pre, path, width
    DW name
    DB pre
    DW path
    DB 0,width,80h
    ENDM
; For direct DLLs the low bit selects the pre-state and bit 7 the row loop.
ENGINE_STREAM = 80h
    DIRECT_ENGINE nameFont4Compact,0,pathFont4Compact,4
    DIRECT_ENGINE nameRot6Stream,ENGINE_STREAM,pathRotRows,6
    DIRECT_ENGINE namePack6Stream,ENGINE_STREAM+1,pathFontPack,6
    DIRECT_ENGINE nameFast6Stream,ENGINE_STREAM+1,pathFontFull,6
    DIRECT_ENGINE namePack4Stream,ENGINE_STREAM,pathFont4,4
    DIRECT_ENGINE nameFast4Stream,ENGINE_STREAM+1,pathFont4Fast,4
    DIRECT_ENGINE nameComp4Stream,ENGINE_STREAM,pathFont4Compact,4
    ENGINE nameOrionM1,0,pathOrionM1,0,6
engineRecordsEnd:
ENGINE_COUNT = (engineRecordsEnd-engineRecords)/8
nameBios:       DB 'BIOS',0
nameMonitor:    DB 'MXOS1',0
nameOpt:        DB 'OPT',0
nameFull:       DB 'SCALAR12',0
nameCache:      DB 'CACHE',0
nameQuad:       DB 'QUAD',0
nameCold:       DB 'COLD',0
nameWarm:       DB 'WARM',0
nameRotate:     DB 'ROTATE',0
nameHalf:       DB 'HALF',0
namePacked:     DB 'PACKED',0
namePaged:      DB 'ROW6',0
nameEight:      DB 'ROW8',0
nameFour:       DB '4RAW',0
nameFourPack:   DB '4PACK',0
nameFourDup:    DB '4DUP',0
path6:          DB 'Engine6.dll',0
pathP:          DB 'EngineP.dll',0
path4:          DB 'Engine4.dll',0
nameRemap:      DB 'REMAP32',0
nameBlock:      DB 'BLOCK32',0
pathM:          DB 'EngineM.dll',0
pathB:          DB 'EngineB.dll',0

nameMon2:       DB 'MON2',0
nameRamfos:     DB 'RAMFOS',0
nameLinear:     DB 'ROTLINE',0
nameFullFast:   DB 'FULLFAST',0
pathS:          DB 'EngineS.dll',0

nameColPre:         DB 'COL6PRE',0
nameTriple:         DB 'TRIPACK',0
pathQ:              DB 'EngineQ.dll',0
nameFontRot:        DB '6ROTATEQUADS',0
pathFontRot:        DB 'FontRot.dll',0
nameFontPack:       DB '6PACKEDQUADS',0
pathFontPack:       DB 'FontPack.dll',0
nameFontFull:       DB '6FULLQUADS',0
pathFontFull:       DB 'FontFull.dll',0
nameFontKoi:        DB 'COMPACT6',0
pathFontKoi:        DB 'FontKoi.dll',0
nameFont8:          DB 'FAST8',0
pathFont8:          DB 'Font8.dll',0
nameFont4:          DB 'CPM4',0
pathFont4:          DB 'Font4.dll',0
nameFont4Fast:      DB 'FAST4',0
pathFont4Fast:      DB 'Font4Fst.dll',0
nameWindow:         DB 'WIN8',0
pathWindow:         DB 'EngineW.dll',0
nameRotRows:        DB '6ROTATEROWS',0
pathRotRows:        DB 'RotRows.dll',0
nameRot6Col4:       DB '6ROTATECOL4',0
pathRot6Col4:       DB 'Rot6Col4.dll',0
nameRot8Quad:       DB '8ROTATEQUADS',0
pathRot8Quad:       DB 'Rot8Quad.dll',0
nameRot8Stream:     DB '8ROTATESTREAM',0
pathRot8Stream:     DB 'Rot8Strm.dll',0
nameFont4Compact:   DB '4COMPACT',0
pathFont4Compact:   DB 'Font4Cmp.dll',0
nameRot6Stream:     DB '6ROTATESTREAM',0
namePack6Stream:    DB '6PACKEDSTREAM',0
nameFast6Stream:    DB '6FASTSTREAM',0
namePack4Stream:    DB '4PACKEDSTREAM',0
nameFast4Stream:    DB '4FASTSTREAM',0
nameComp4Stream:    DB '4COMPACTSTREAM',0
nameOrionM1:        DB 'ORIONM1',0
pathOrionM1:        DB 'OrionM1.dll',0
enginePath:         DB 'H:/Bench/FontBen/'
engineBasename:
    DS MAX_PATH-($-enginePath),0 ; DOS copies the full path buffer.
