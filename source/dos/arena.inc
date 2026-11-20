; Exact half-open intervals in DOS. No fixed-size slots or rounding.
; Loaded DLLs register start/end in MODULE_TABLE; an active installer and the
; cached FILEOPS/ENUM images are the extra intervals. Active code is pinned.
; HL=start, DE=end; BC/DE/HL preserved. CF/A=ACTIVE on an intersection.
; Cached, idle overlays may be discarded, but never a live DLL or installer.
dosArenaImage:
    ld a,(v_imageHeader+IMAGE_POLICY)
    jp arenaEnter
dosArenaCheck:
    ld a,IMAGE_POLICY_MANAGED
arenaEnter:
    ld (arenaPolicy),a
    push bc
    push de
    push hl
    ld (arenaStart),hl
    ex de,hl
    ld (arenaEnd),hl
    call imageSaveContext
    xor a
    ld (arenaPhase),a
arenaRestart:
    ld a,(diActive)
    or a
    jp z,arenaModules
    ld hl,(diStart)
    MD_GET_DE diEnd
    call arenaOverlap
    jp nz,arenaConflict
arenaModules:
    ld hl,MODULE_TABLE
    ld b,MODULE_MAX
arenaNext:
    ld a,(hl)
    inc hl
    or (hl)
    dec hl
    jp z,arenaSkip
    push hl
    ld de,3
    add hl,de
    ld a,(hl)
    cp DOS_IO_PAGE
    pop hl
    jp nz,arenaSkip
    push hl
    ld de,10
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    push de
    ld e,(hl)
    inc hl
    ld d,(hl)
    pop hl
    call arenaOverlap
    pop hl
    jp z,arenaSkip
    push bc
    push hl
    inc hl
    inc hl
    ld a,(hl)
    or a
    jp nz,arenaModulePinned
    ld a,(arenaPolicy)
    cp IMAGE_POLICY_PRESERVE
    jp z,arenaModulePinned
    ld de,12
    add hl,de
    ld a,(hl)
    and IMAGE_OPTION_CACHEABLE
    jp z,arenaModulePinned
    pop hl
    push hl
    call mdCanRelease
    jp c,arenaGuardFailed
    ld a,(arenaPhase)
    or a
    call nz,mdReleaseChecked
arenaGuardFailed:
    pop hl
    pop bc
    jp c,arenaConflict
arenaSkip:
    ld de,MODULE_RECORD_SIZE
    add hl,de
    dec b
    jp nz,arenaNext
    ld hl,fileopsOverlay
    call arenaCached
    jp c,arenaConflict
    ld hl,enumOverlay
    call arenaCached
    jp c,arenaConflict
    ld a,(arenaPhase)
    or a
    jp nz,arenaFree
    inc a
    ld (arenaPhase),a
    jp arenaRestart
arenaModulePinned:
    pop hl
    pop bc
    jp arenaConflict
arenaFree:
    call imageRestoreContext
    pop hl
    pop de
    pop bc
    xor a
    ret
arenaConflict:
    call imageRestoreContext
    pop hl
    pop de
    pop bc
    ld a,MEM_ERROR_ACTIVE
    jp dosSetError
; HL=kernel-owned cache descriptor. Preserve BC, CF=active intersection.
arenaCached:
    ld (arenaCacheRecord),hl
    ld de,OV_READY
    add hl,de
    ld a,(hl)
    or a
    ret z
    ld hl,(arenaCacheRecord)
    ld e,(hl)
    inc hl
    ld d,(hl)
    push de
    ld de,OV_END-1
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    pop hl
    call arenaOverlap
    ret z
    ld hl,(arenaCacheRecord)
    ld de,OV_ACTIVE
    add hl,de
    ld a,(hl)
    or a
    scf
    ret nz
    ld a,(arenaPolicy)
    cp IMAGE_POLICY_PRESERVE
    scf
    ret z
    ld a,(arenaPhase)
    or a
    ret z
    dec hl
    ld (hl),0
    or a
    ret
arenaCacheRecord: DW 0
; HL=otherStart, DE=otherEnd. NZ=overlap, Z=disjoint, BC preserved.
arenaOverlap:
    push hl
    ex de,hl
    MD_GET_DE arenaStart
    call cmp_hl_de
    pop hl
    jp c,arenaApart
    jp z,arenaApart
    MD_GET_DE arenaEnd
    call cmp_hl_de
    jp nc,arenaApart
    ld a,1
    or a
    ret
arenaApart:
    xor a
    ret
arenaStart: DW 0
arenaEnd: DW 0

arenaPhase: DB 0
arenaPolicy: DB IMAGE_POLICY_MANAGED
