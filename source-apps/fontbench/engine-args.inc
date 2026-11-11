; Options select engines, not callers: every selected engine runs the same
; applicable MAIN/BANK grid. The first -E replaces the default BIOS choice;
; subsequent -E and comma-separated names add to it.
gridArgE:
    ld a,(hl)
    cp '='
    jp z,engineArgESep
    cp ' '
    jp nz,gridUsage
engineArgESep:
    inc hl
    push hl
    ld a,(engineFirstArg)
    or a
    jp nz,engineArgStarted
    call engineSelectAll
    ld a,1
    ld (engineFirstArg),a
engineArgStarted:
    pop hl
engineArgNext:
    call engineToken
    ret c
    push hl
    ld hl,argToken
    ld de,argAll
    call engineEqual
    jp z,engineArgAll
    ld hl,argToken
    ld de,argStandalone
    call engineEqual
    jp z,engineArgStandalone
    ld hl,engineRecords
    ld b,ENGINE_COUNT
engineArgFind:
    push hl
    push bc
    ld e,(hl)
    inc hl
    ld d,(hl)
    ld hl,argToken
    call engineEqual
    pop bc
    pop hl
    jp z,engineArgFound
    ld de,8
    add hl,de
    dec b
    jp nz,engineArgFind
    pop hl
    jp gridUsage
engineArgFound:
    ld de,5
    add hl,de
    ld (hl),1
    jp engineArgDone
engineArgAll:
    ld a,1
    call engineSelectAll
    jp engineArgDone
engineArgStandalone:
    ld a,1
    call engineSelectAll
    xor a
    ld (engineRecords+5),a
engineArgDone:
    pop hl
    ld a,(hl)
    cp ','
    jp nz,gridArgsNext
    inc hl
    jp engineArgNext
gridArgP:
    ld a,(hl)
    cp '='
    jp z,engineArgPSep
    cp ' '
    jp nz,gridUsage
engineArgPSep:
    inc hl
    call engineToken
    ret c
    push hl
    ld hl,argToken
    ld de,argOff
    call engineEqual
    jp z,engineArgOff
    ld hl,argToken
    ld de,argOn
    call engineEqual
    jp z,engineArgOn
    ld hl,argToken
    ld de,argBoth
    call engineEqual
    jp nz,engineArgInvalid
    xor a
    ld (enginePreFirst),a
    ld a,2
    jp engineArgEnd
engineArgOff:
    xor a
    ld (enginePreFirst),a
    inc a
    jp engineArgEnd
engineArgOn:
    ld a,1
    ld (enginePreFirst),a
    inc a
engineArgEnd:
    ld (enginePreEnd),a
    pop hl
    jp gridArgsNext
engineArgInvalid:
    pop hl
    jp gridUsage
; Exact case-insensitive comparison; letters only are case-folded, retaining
; digits/punctuation. Token consumption leaves HL on space/comma/NUL.
engineToken:
    ld de,argToken
    ld b,15
engineTokenNext:
    ld a,(hl)
    or a
    jp z,engineTokenDone
    cp ' '
    jp z,engineTokenDone
    cp ','
    jp z,engineTokenDone
    cp 'a'
    jp c,engineTokenStore
    cp 'z'+1
    jp nc,engineTokenStore
    and 0DFh
engineTokenStore:
    ld (de),a
    inc de
    inc hl
    dec b
    jp nz,engineTokenNext
    jp gridUsage
engineTokenDone:
    xor a
    ld (de),a
    ret
engineEqual:
    ld a,(de)
    cp (hl)
    ret nz
    or a
    ret z
    inc de
    inc hl
    jp engineEqual
argToken: DS 16,0
argAll: DB 'ALL',0
argStandalone: DB 'STANDALONE',0
argOff: DB 'OFF',0
argOn: DB 'ON',0
argBoth: DB 'BOTH',0
