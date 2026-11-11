; A MAIN renderer DLL owns [2000h,end); the controller and five frontends
; stay below 2000h. Only initialization uses MODULE_CALL, never timed pixels.
; Native BIOS uses the same arena to reserve its 3000h..5800h pre-render.
engineOpen:
    ld b,0
    ld hl,02000h
    ld de,05C00h
    ld a,MEM_OP_ARENA
    MXOS_CALL MXOS_DOS_MEMORY
    ret c
    ld a,(engineId)
    or a
    jp nz,engineLoad
    ld b,0
    ld hl,03800h
    ld a,MEM_OP_ALLOC
    MXOS_CALL MXOS_DOS_MEMORY
    ret c
    ld (engineAllocation),hl
    ld a,(gridPre)
    xor 3
    dec a
    ld hl,03000h
    MXOS_CALL MXOS_BIOS_FONT_RENDER
    ret
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
    xor a
    MXOS_CALL MXOS_DOS_MODULE_LOAD
    ret c
    ld (engineHandle),hl
    ld a,(gridPre)
    ld (engineRegs+5),a
    ld de,engineRegs
    MXOS_CALL MXOS_DOS_MODULE_CALL
    ret c
    ld a,(engineRegs+6)
    rra
    ret c
    ld hl,(engineRegs)
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
    call engineResetStage
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
    ld (bios_vars.cursorY),hl
    ld hl,gridText
    ld bc,1152
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
    ld a,2
    MXOS_CALL MXOS_BIOS_FONT_RENDER
    ld hl,(engineHandle)
    ld a,h
    or l
    jp z,engineFree
    xor a
    MXOS_CALL MXOS_DOS_MODULE_UNLOAD
    ld hl,0
    ld (engineHandle),hl
engineFree:
    ld hl,(engineAllocation)
    ld a,h
    or l
    ret z
    ld b,0
    ld a,MEM_OP_FREE
    MXOS_CALL MXOS_DOS_MEMORY
    ld hl,0
    ld (engineAllocation),hl
    ret
; Eight-byte registry record: name, variant, DLL path, selected byte, width.
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
engineId: DB 0
engineWidth: DB 6
engineHooked: DB 0
engineHandle: DW 0
engineAllocation: DW 0
engineRegs: DS 8,0
engineFirstArg: DB 0
enginePreFirst: DB 0
enginePreEnd: DB 2
engineFileAppend: DB 0
    MACRO ENGINE name, variant, path, selected, width
    DW name
    DB variant
    DW path
    DB selected,width,0
    ENDM
engineRecords:
    ENGINE nameBios,0,0,1,6
    ENGINE nameMonitor,0,path6,0,6
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
    ENGINE nameMon2,0,pathS,0,6
    ENGINE nameRamfos,1,pathS,0,6
    ENGINE nameLinear,2,pathS,0,6
    ENGINE nameFullFast,3,pathS,0,6
    ENGINE nameColPre,4,pathS,0,6
ENGINE_COUNT = ($-engineRecords)/8
nameBios: DB 'BIOS',0
nameMonitor: DB 'MXOS1',0
nameOpt: DB 'OPT',0
nameFull: DB 'FULL',0
nameCache: DB 'CACHE',0
nameQuad: DB 'QUAD',0
nameCold: DB 'COLD',0
nameWarm: DB 'WARM',0
nameRotate: DB 'ROTATE',0
nameHalf: DB 'HALF',0
namePacked: DB 'PACKED',0
namePaged: DB 'PAGED',0
nameEight: DB '8X8',0
nameFour: DB '4RAW',0
nameFourPack: DB '4PACK',0
nameFourDup: DB '4DUP',0
path6: DB 'A:/Bench/FontBen/Engine6.dll',0
    DS MAX_PATH-($-path6),0
pathP: DB 'A:/Bench/FontBen/EngineP.dll',0
    DS MAX_PATH-($-pathP),0
path4: DB 'A:/Bench/FontBen/Engine4.dll',0
    DS MAX_PATH-($-path4),0
nameRemap: DB 'REMAP32',0
nameBlock: DB 'BLOCK32',0
pathM: DB 'A:/Bench/FontBen/EngineM.dll',0
    DS MAX_PATH-($-pathM),0
pathB: DB 'A:/Bench/FontBen/EngineB.dll',0
    DS MAX_PATH-($-pathB),0

nameMon2: DB 'MON2',0
nameRamfos: DB 'RAMFOS',0
nameLinear: DB 'ROTLINE',0
nameFullFast: DB 'FULLFAST',0
pathS: DB 'A:/Bench/FontBen/EngineS.dll',0
    DS MAX_PATH-($-pathS),0

nameColPre: DB 'COLPRE',0
