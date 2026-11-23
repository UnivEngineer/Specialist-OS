; Transactional MAIN claim: inspect every overlap before releasing anything.
; Exact public requests preserve the caller's own allocations. Loader requests
; use the image policy and full declared interval, including BSS/below-load data.
memoryMainClaim:
    push bc
    push de
    push hl
    ld a,IMAGE_POLICY_MANAGED
    ld (mmcPolicy),a
    ld a,1
    ld (mmcAllowOwner),a
    ld (mmcStart),hl
    add hl,bc
    jp c,mmcRange
    ld de,APP_MEMORY_END
    call cmp_hl_de
    jp c,mmcPublicEnd
    jp nz,mmcRange
mmcPublicEnd:
    ld (mmcEnd),hl
    ld a,b
    cp APP_MEMORY_END/256
    jp nz,mmcPublicPolicyReady
    ld a,c
    or a
    jp nz,mmcPublicPolicyReady
    ld a,IMAGE_POLICY_EXCLUSIVE
    ld (mmcPolicy),a
mmcPublicPolicyReady:
    jp mmcBegin
moduleMainClaimStrict:
    xor a
    jp mmcLoader
moduleMainClaim:
    ld a,1
mmcLoader:
    ld (mmcAllowOwner),a
    push bc
    push de
    push hl
    ld a,(v_imageHeader+IMAGE_POLICY)
    ld (mmcPolicy),a
    or a
    jp z,mmcFullMain
    ld hl,(v_imageHeader+IMAGE_MEMORY_BEGIN)
    ld (mmcStart),hl
    ld hl,(v_imageHeader+IMAGE_MEMORY_END)
    push hl
    ld hl,(v_imageHeader+IMAGE_MIN_HEAP)
    ex de,hl
    pop hl
    add hl,de
    ld (mmcEnd),hl
    jp mmcBegin
mmcFullMain:
    ld hl,0
    ld (mmcStart),hl
    ld hl,APP_MEMORY_END
    ld (mmcEnd),hl
mmcBegin:
    ld a,(mmcPolicy)
    or a
    jp z,mmcBeginReady
    ld hl,(mmcEnd)
    MD_GET_DE memorySystemEnd
    call cmp_hl_de
    jp c,mmcBeginReady
    jp nz,mmcRange
mmcBeginReady:
    call imageSaveContext
    xor a
    ld (mmcPhase),a
mmcRestart:
    ld hl,MODULE_TABLE
    ld b,MODULE_MAX
mmcModule:
    ld a,(hl)
    inc hl
    or (hl)
    dec hl
    jp z,mmcNext
    push hl
    ld de,3
    add hl,de
    ld a,(hl)
    pop hl
    or a
    jp nz,mmcNext
    push bc
    push hl
    ld de,9
    add hl,de
    ld a,(hl)
    ld (mmcOwner),a
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld c,(hl)
    inc hl
    ld b,(hl)
    call mmcOverlap
    jp c,mmcRelease
    ld hl,MEM_ALLOC_TABLE
    ld b,MEM_ALLOC_RECORDS
mmcBlock:
    ld a,(hl)
    or a
    jp nz,mmcBlockNext
    push hl
    push bc
    call mmcReadBlock
    ld a,(mmcOwner)
    cp (hl)
    jp nz,mmcNotOwned
    call mmcOverlapSize
    jp c,mmcOwnedOverlap
mmcNotOwned:
    pop bc
    pop hl
mmcBlockNext:
    ld de,6
    add hl,de
    dec b
    jp nz,mmcBlock
    pop hl
    pop bc
    jp mmcNext
mmcOwnedOverlap:
    pop bc
    pop hl
mmcRelease:
    pop hl
    push hl
    inc hl
    inc hl
    ld a,(hl)
    or a
    jp nz,mmcActive
    ld a,(mmcPolicy)
    cp IMAGE_POLICY_PRESERVE
    jp z,mmcActive
    or a
    jp z,mmcMayRelease
    ld de,12
    add hl,de
    ld a,(hl)
    and IMAGE_OPTION_CACHEABLE
    jp z,mmcActive
mmcMayRelease:
    pop hl
    push hl
    call mdCanRelease
    jp c,mmcGuardFailed
    ld a,(mmcPhase)
    or a
    call nz,mdReleaseChecked
mmcGuardFailed:
    pop hl
    pop bc
    jp c,mmcDone
mmcNext:
    ld de,MODULE_RECORD_SIZE
    add hl,de
    dec b
    jp nz,mmcModule
    ; Track arbitrary MAIN blocks as well as DLL intervals.
    ld hl,MEM_ALLOC_TABLE
    ld b,MEM_ALLOC_RECORDS
mmcAllocation:
    ld a,(hl)
    or a
    jp nz,mmcAllocationNext
    push hl
    push bc
    call mmcReadBlock
    ld a,(hl)
    ld (mmcOwner),a
    call mmcOverlapSize
    jp nc,mmcAllocationPop
    ld a,(mmcOwner)
    cp 0FFh                 ; private DLL image, checked above
    jp z,mmcAllocationPop
    ld c,a
    ld a,(mmcAllowOwner)
    or a
    jp z,mmcForeignBlock
    call memoryOwnerCurrent
    cp c
    jp z,mmcAllocationPop
mmcForeignBlock:
    ; A MAIN module's owned BSS was included in the module pass.
    ld a,c
    cp MODULE_OWNER_BASE
    jp c,mmcOrdinaryBlock
    sub MODULE_OWNER_BASE
    cp MODULE_MAX
    jp nc,mmcOrdinaryBlock
    add a,a
    add a,a
    add a,a
    add a,a
    ld l,a
    ld h,0
    ld de,MODULE_TABLE+3
    add hl,de
    ld a,(hl)
    or a
    jp z,mmcAllocationPop
mmcOrdinaryBlock:
    ld a,c
    cp APP_OWNER_BASE
    jp c,mmcNonApp
    cp APP_OWNER_BASE+APP_MAX_DEPTH
    jp c,mmcAllocationActive
mmcNonApp:
    ld a,(mmcPolicy)
    or a
    jp nz,mmcAllocationActive
    ld a,(mmcPhase)
    or a
    jp z,mmcAllocationPop
    pop bc
    pop hl
    ld (hl),0FFh
    jp mmcAllocationNext
mmcAllocationPop:
    pop bc
    pop hl
mmcAllocationNext:
    ld de,6
    add hl,de
    dec b
    jp nz,mmcAllocation
    ld a,(mmcPhase)
    or a
    jp nz,mmcSuccess
    ld a,(mmcPolicy)
    or a
    call z,mainRestoreFont
    jp c,mmcDone
    ld a,1
    ld (mmcPhase),a
    jp mmcRestart
mmcSuccess:
    ld hl,(mmcEnd)
    ex de,hl
    ld hl,(mmcStart)
    call mdReuseInvalidate
    call memoryPublishTop
    xor a
mmcDone:
    push af
    call imageRestoreContext
    pop af
    pop hl
    pop de
    pop bc
    ret
mmcAllocationActive:
mmcActive:
    pop hl
    pop bc
mmcError:
    ld a,MEM_ERROR_ACTIVE
    scf
    jp mmcDone
mmcRange:
    pop hl
    pop de
    pop bc
    jp memoryBad
; Read a MAIN allocation. HL advances from the bank byte to the owner.
mmcReadBlock:
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld c,(hl)
    inc hl
    ld b,(hl)
    inc hl
    ret
; DE=other start, BC=other size.
mmcOverlapSize:
    ld h,b
    ld l,c
    add hl,de
    ld b,h
    ld c,l
; DE=other start, BC=other end. CF=intersection of half-open intervals.
mmcOverlap:
    ld hl,(mmcEnd)
    call cmp_hl_de
    jp z,mmcSeparate
    jp c,mmcSeparate
    ld hl,(mmcStart)
    ld d,b
    ld e,c
    jp cmp_hl_de
mmcSeparate:
    or a
    ret

; A whole-MAIN launch releases a lower font prerender. The system font is
; trusted: load it directly into its fixed slot, using the normal image loader.
; No staging allocation, reread or separate verification pass.
mainRestoreFont:
    ld bc,MEM_RESIDENT_FONT
    call memoryResident
    ret c
    ld a,h
    or l
    ret z
    ld a,(kdPage)
    ld (mfCaller),a
    ld hl,mfFontPath
    ld de,v_curFileDescr
    call fileNamePrepare
    jp c,mfDone
    ld hl,v_curFileDescr
    call imageOpen
    jp c,mfDone
    ld hl,(v_tmpFileDescr+6)
    ld (kdFarCurrent),hl
    ld hl,0
    ld (kiSum),hl
    xor a
    ld (kdPage),a
    inc a
    ld (kiSumActive),a
    ld (kdFarActive),a
    ld de,KD_DATA
    call imageLoad
    push af
    xor a
    ld (kiSumActive),a
    ld (kdFarActive),a
    pop af
    jp c,mfDone
    ld hl,FONT_ENGINE_INIT
    ld de,0
    xor a
    call bankCall
    ld hl,mfFallbackName
    ld de,io_vars.fontName
    ld bc,8
    call kdStore
    ld bc,0100h+MEM_RESIDENT_FONT
    ld hl,0
    ld de,0
    call memoryResident
    ld a,(fontConfigValid)
    ld (fontConfigPending),a
    xor a
mfDone:
    push af
    ld a,(mfCaller)
    ld (kdPage),a
    pop af
    ret
mfFontPath:         DB BOOT_VOLUME_LETTER,':',05Ch,'FONTS',05Ch,'6X8',05Ch,'ROTATE.FNT',0
mfFallbackName:     DB "Rotate  "
