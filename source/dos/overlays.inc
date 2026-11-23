; Shared, non-reentrant loader for matched system overlays.
; Record: base,limit,name,host (words), hostBytes,ready,active (bytes),end (word).
; Caller pins its descriptor while executing. Loader leaves filesystem context.
OV_READY  = 9
OV_ACTIVE = 10
OV_END    = 11
overlayEnsure:
    ld (ovRecord),hl
    ld e,OV_READY
    call ovByte
    or a
    ret nz
    ld hl,(v_directory)
    push hl
    ld a,(v_drive)
    push af
    ld hl,v_directories
    ld de,ovDirectories
    ld bc,16
    call moveBytesForward
    ld e,4
    call ovWord
    ld de,v_curFileDescr
    call fileNamePrepare
    jp c,ovLoadDone
    ld hl,v_curFileDescr
    call imageOpen
    jp c,ovLoadDone
    ld a,(v_tmpFileDescr+5)
    cp IMAGE_FLAG_DRIVER
    jp nz,ovBadImage
    push hl                 ; Entry must equal this overlay's fixed origin.
    push de
    ld e,0
    call ovWord
    pop de
    call cmp_hl_de
    pop de
    jp nz,ovBadImage
    call cmp_hl_de
    jp nz,ovBadImage
    ld hl,(v_imageHeader+IMAGE_MEMORY_BEGIN)
    call cmp_hl_de
    jp nz,ovBadImage
    ld hl,(v_imageHeader+IMAGE_MEMORY_END)
    ld (ovImageEnd),hl
    push hl
    ld e,2
    call ovWord
    pop de
    call cmp_hl_de
    jp c,ovBadImage
    ld e,0
    call ovWord
    push hl
    ld hl,(ovImageEnd)
    ex de,hl
    pop hl
    call dosArenaImage
    jp c,ovLoadDone
    ld hl,(ovRecord)
    ld de,OV_END
    add hl,de
    push hl
    ld hl,(ovImageEnd)
    ex de,hl
    pop hl
    ld (hl),e
    inc hl
    ld (hl),d
    ld e,0
    call ovWord
    ex de,hl
    call imageLoad
    jp c,ovLoadDone
    ld e,8
    call ovByte
    ld b,a
    ld e,6
    call ovWord
    push hl
    ld e,0
    call ovWord
    inc hl
    inc hl
    inc hl
    ex de,hl
    pop hl
ovCheckHost:
    ld a,(de)
    cp (hl)
    jp nz,ovBadImage
    inc hl
    inc de
    dec b
    jp nz,ovCheckHost
    ld hl,(ovRecord)
    ld de,OV_READY
    add hl,de
    ld (hl),1
    xor a
    jp ovLoadDone
ovBadImage:
    ld a,DOS_ERR_BAD_IMAGE
    call dosSetError
ovLoadDone:
    push af
    ld a,(v_dosError)
    push af
    pop bc
    pop de
    pop af
    push de
    push bc
    call fileSelectDrive
    pop bc
    pop de
    pop hl
    ld (v_directory),hl
    push de
    push bc
    ld hl,ovDirectories
    ld de,v_directories
    ld bc,16
    call moveBytesForward
    pop bc
    pop de
    ld a,b
    ld (v_dosError),a
    push de
    pop af
    ret
ovAddress:
    ld hl,(ovRecord)
    ld d,0
    add hl,de
    ret
ovByte:
    call ovAddress
    ld a,(hl)
    ret
ovWord:
    call ovAddress
    ld a,(hl)
    inc hl
    ld h,(hl)
    ld l,a
    ret
overlayBusy:
    ld a,MEM_ERROR_ACTIVE
    jp dosSetError

fileopsName: DB BOOT_VOLUME_LETTER,':',05Ch,"DOS",05Ch,"FILEOPS.DLL",0
fileopsHost:
    INCLUDE "../include/fileops-host.inc"
fileopsHostEnd:
    ASSERT fileopsHostEnd-fileopsHost < 256

enumName: DB BOOT_VOLUME_LETTER,':',05Ch,"DOS",05Ch,"ENUM.DLL",0
enumHost:
    INCLUDE "../include/enumerate-host.inc"
enumHostEnd:
    ASSERT enumHostEnd-enumHost < 256
