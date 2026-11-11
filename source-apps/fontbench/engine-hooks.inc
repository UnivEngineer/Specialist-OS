; Frontends shared by every standalone backend. No hooks in the BIOS run.
; Public Char/Raw preserve all registers; strings preserve AF/BC/DE and return
; the first unread HL. Scalar backends may clobber registers; batch backends
; take HL/C=bounded run and preserve BC/DE/HL. No duplicate saves in helpers.
; Saving/restoring only public JMPs would miss COMMAND and *_MAIN MXOS_CALL slots.
engineSave:
    ld hl,bios_printChar
    ld de,originalChar
    ld bc,3
    call gridCopy
    ld hl,bios_printCharRaw
    ld de,originalRaw
    ld bc,3
    call gridCopy
    ld hl,hookSlots
    ld de,savedSlots
    ld a,13
    ld (hookLeft),a
saveSlot:
    ld c,(hl)
    inc hl
    ld b,(hl)
    inc hl
    ld a,(bc)
    ld (de),a
    inc bc
    inc de
    ld a,(bc)
    ld (de),a
    inc de
    ld a,(hookLeft)
    dec a
    ld (hookLeft),a
    jp nz,saveSlot
    ret
engineInstall:
    ld hl,hookTargets
    ex de,hl
    ld hl,hookSlots
    jp writeSlots
engineRestore:
    ld de,savedSlots
    ld hl,hookSlots
writeSlots:
    ld a,13
    ld (hookLeft),a
writeSlot:
    ld c,(hl)
    inc hl
    ld b,(hl)
    inc hl
    ld a,(de)
    ld (bc),a
    inc bc
    inc de
    ld a,(de)
    ld (bc),a
    inc de
    ld a,(hookLeft)
    dec a
    ld (hookLeft),a
    jp nz,writeSlot
    ret
originalChar: jp bios_printChar
originalRaw: jp bios_printCharRaw
hookChar:
    push af
    push bc
    push de
    push hl
    ld a,(bios_upper.escMode)
    inc a
    jp nz,hookControl
    ld a,c
    cp 20h
    jp c,hookControl
    call engineScalar
    jp hookScalarWrap
hookControl:
    call originalChar
    jp hookScalarDone
hookRaw:
    push af
    push bc
    push de
    push hl
    ld c,a
    call engineScalar
hookScalarWrap:
    call engineWrap
hookScalarDone:
    pop hl
    pop de
    pop bc
    pop af
    ret
engineScalar: jp 0
engineBatch: jp 0
engineResetStage: jp engineEmpty
engineEmpty: ret
engineWrap:
    ld a,(bios_vars.cursorX)
    cp 192
    ret c
    push bc
    ld c,10
    call originalChar
    pop bc
    ret
hookZ:
    push af
    xor a
    jp hookString
hookRawZ:
    push af
    ld a,1
    jp hookString
hookLim:
    push af
    ld a,2
hookString:
    ld (hookMode),a
    push bc
    push de
    ld a,b
    ld (hookRemaining+1),a
    ld a,c
    ld (hookRemaining),a
hookNext:
    ld a,(hookMode)
    cp 2
    jp z,hookCountCheck
    ld a,(hl)
    or a
    jp z,hookDone
    ld a,(hookMode)
    or a
    jp nz,hookTryBatch
    ld a,(bios_upper.escMode)
    inc a
    jp nz,hookOneControl
    ld a,(hl)
    cp 20h
    jp c,hookOneControl
    jp hookTryBatch
hookCountCheck:
    push hl
    ld hl,(hookRemaining)
    ld a,h
    or l
    pop hl
    jp z,hookDone
hookTryBatch:
    ld a,(bios_vars.cursorX)
    and 3
    jp nz,hookOne
    ; Byte-column lookup yields a run fitting the screen, capped at 48.
    ; The table depends on width; no repeated division in the hot path.
    push hl
    ld a,(bios_vars.cursorX)
    rrca
    rrca
    and 63
    ld l,a
    ld h,0
hookBounds: ld de,bounds6
    add hl,de
    ld c,(hl)
    pop hl
    ld a,c
    or a
    jp z,hookOne
    ld a,(hookMode)
    cp 2
    jp z,hookBoundCount
    ; Z inspects at most the bounded run, not the entire client string.
    push hl
    ld b,c
    ld c,0
    ld a,(hookMode)
    or a
    ld a,1
    jp nz,hookThreshold
    ld a,20h
hookThreshold:
    ld (hookMin+1),a
hookScan:
    ld a,(hl)
hookMin: cp 1
    jp c,hookScanned
    inc hl
    inc c
    dec b
    jp nz,hookScan
hookScanned:
    pop hl
    jp hookRound
hookBoundCount:
    ld a,(hookRemaining+1)
    or a
    jp nz,hookRound
    ld a,(hookRemaining)
    cp c
    jp nc,hookRound
    ld c,a
hookRound:
    ld a,c
hookRoundMask: and 0FCh
    ld c,a
    or a
    jp z,hookOne
    ; Batch and wrap preserve BC/HL: no extra save around this call.
    call engineBatch
    call engineWrap
    ld b,0
    add hl,bc
    jp hookConsumed
hookOneControl:
    ld c,(hl)
    push hl
    call originalChar
    pop hl
    jp hookOneDone
hookOne:
    ld c,(hl)
    push hl
    call engineScalar
    call engineWrap
    pop hl
hookOneDone:
    inc hl
    ld bc,1
hookConsumed:
    ld a,(hookMode)
    cp 2
    jp nz,hookNext
    push hl
    ld hl,(hookRemaining)
    ld a,l
    sub c
    ld l,a
    ld a,h
    sbc a,b
    ld h,a
    ld (hookRemaining),hl
    pop hl
    jp hookNext
hookDone:
    pop de
    pop bc
    pop af
    ret
hookMode: DB 0
hookLeft: DB 0
hookRemaining: DW 0
; Fixed JMP operand plus all dispatcher aliases for the five public APIs.
    MACRO HOOK_SLOT abi, id, idMain
    DW abi+1,BASE_BIOS_TABLE+2*(id-0100h)
    IF idMain != 0
    DW BASE_BIOS_TABLE+2*(idMain-0100h)
    ENDIF
    ENDM
hookSlots:
    HOOK_SLOT bios_printChar,MXOS_BIOS_PRINT_CHAR,0
    HOOK_SLOT bios_printCharRaw,MXOS_BIOS_PRINT_CHAR_RAW,0
    HOOK_SLOT bios_printStringZ,MXOS_BIOS_PRINT_STRING_Z,MXOS_BIOS_PRINT_STRING_Z_MAIN
    HOOK_SLOT bios_printStringRawZ,MXOS_BIOS_PRINT_STRING_RAW_Z,MXOS_BIOS_PRINT_STRING_RAW_Z_MAIN
    HOOK_SLOT bios_printStringRawLim,MXOS_BIOS_PRINT_STRING_RAW_LIM,MXOS_BIOS_PRINT_STRING_RAW_LIM_MAIN
hookTargets:
    DW hookChar,hookChar,hookRaw,hookRaw
    DW hookZ,hookZ,hookZ,hookRawZ,hookRawZ,hookRawZ,hookLim,hookLim,hookLim
savedSlots: DS 26,0
bounds6:
    DB 48,48,48,48,48,48,48,48,48,48,48,48,48,44,44,44,40,40,40,36,36,36,32,32,32,28,28,28,24,24,24,20,20,20,16,16,16,12,12,12,8,8,8,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
bounds4:
    DB 48,48,48,48,48,48,48,48,48,48,48,48,48,48,48,48,48,48,48,48,48,48,48,48,48,46,44,42,40,38,36,34,32,30,28,26,24,22,20,18,16,14,12,10,8,6,4,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
bounds8:
    DB 48,47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
