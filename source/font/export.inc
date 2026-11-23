; Import export is outside all timed/render-critical paths. Save the staged
; image (not the running, self-modified slot), its raw PRE, and pristine init.
; No filename is chosen from a fixed drive letter: probe mounted writable RAM.
saveImported:
    ld a,(importPath)
    or a
    ret z
    ld a,(outputPath)
    or a
    jp nz,exportChosen
    xor a
    ld (exportDrive),a
exportProbe:
    ld a,(exportDrive)
    MXOS_CALL MXOS_DOS_FILE_SELECT_DRIVE
    jp c,exportNextDrive
    ld b,0FFh
    MXOS_CALL MXOS_DOS_RAW_IO
    jp c,exportNextDrive
    ld a,b
    cp DISK_TECH_RAM
    jp nz,exportNextDrive
    ld a,c
    and DISK_CAP_WRITE
    jp z,exportNextDrive
    ld a,(exportDrive)
    add a,'A'
    ld (outputPath),a
    ld a,':'
    ld (outputPath+1),a
    ld a,05Ch
    ld (outputPath+2),a
    ; Default basename comes from the flat input, never the engine template.
    ld hl,importPath
    call exportLeaf
    ld de,outputPath+3
    call copyPath
    jp exportChosen
exportNextDrive:
    ld hl,exportDrive
    inc (hl)
    ld a,(hl)
    cp 8
    jp c,exportProbe
    ld a,(oldDrive)
    MXOS_CALL MXOS_DOS_FILE_SELECT_DRIVE
    ld hl,exportNoRam
    call bios_printStringZ
    xor a
    ret
exportChosen:
    ld a,(oldDrive)
    MXOS_CALL MXOS_DOS_FILE_SELECT_DRIVE ; Replace a supplied extension, or append .fnt to a bare output prefix.
    ld hl,outputPath
    call exportLeaf
exportStem:
    ld a,(hl)
    or a
    jp z,exportExtension
    cp '.'
    jp z,exportExtension
    inc hl
    jp exportStem
exportExtension:
    ld de,extension
    call copyToHL
    ; Do not let a user basename overwrite the companion initializer.
    ld hl,outputPath
    call exportLeaf
    push hl
    ld hl,initializerPath
    call exportLeaf
    ex de,hl
    pop hl
    call equalOption
    jp z,badImage
    ; Native executable checksum is needed for reloading this changed image.
    ld hl,(imageBytes)
    ld de,staging+IMAGE_HEADER_SIZE-1
    add hl,de
    ex de,hl
    ld hl,staging+IMAGE_HEADER_SIZE
    call bios_calcCS
    ld h,b
    ld l,c
    ld (staging+12),hl
    ld hl,staging
    ld de,staging+IMAGE_HEADER_CHECKSUM-1
    call bios_calcCS
    ld h,b
    ld l,c
    ld (staging+IMAGE_HEADER_CHECKSUM),hl
    ld hl,outputPath
    ld (exportIO+FILE_PATH_IO.path),hl
    ld hl,staging
    ld (exportIO+FILE_PATH_IO.buffer),hl
    ld hl,(imageBytes)
    ld de,IMAGE_HEADER_SIZE
    add hl,de
    ld (exportIO+FILE_PATH_IO.count),hl
    call exportWrite
    ret c
    ld hl,outputPath
    ld de,exportAuxPath
    call copyPath
    ld hl,(staging+IMAGE_HEADER_SIZE+44)
    ld a,h
    or l
    jp z,exportInitializer
    ld (exportIO+FILE_PATH_IO.count),hl
    ld hl,(lowerBase)
    ld (exportIO+FILE_PATH_IO.buffer),hl
    ld hl,exportAuxPath
    call findExtension
    ld de,preExtension
    call copyToHL
    ld hl,exportAuxPath
    ld (exportIO+FILE_PATH_IO.path),hl
    call exportWrite
    ret c
exportInitializer:
    ; Init6 modifies its temporary state while importing. Reload its original
    ; container before copying it, preserving its native checksum verbatim.
    ld hl,initializerIO
    call loadPath
    ret c
    ld hl,exportAuxPath
    call exportLeaf
    push hl
    ld hl,initializerPath
    call exportLeaf
    pop de
    call copyPath
    ; The selected output folder may already be the template folder.
    ; Do not rewrite its shared (possibly read-only) initializer in place.
    ld hl,exportAuxPath
    ld de,initializerPath
    call equalOption
    jp z,exportFinished
    ld hl,exportAuxPath
    ld (exportIO+FILE_PATH_IO.path),hl
    ld hl,FONT_INIT_ADDR-IMAGE_HEADER_SIZE
    ld (exportIO+FILE_PATH_IO.buffer),hl
    ld hl,(initializerIO+FILE_PATH_IO.actual)
    ld (exportIO+FILE_PATH_IO.count),hl
    call exportWrite
    ret c
exportFinished:
    ld hl,exportSaved
    call bios_printStringZ
    ld hl,outputPath
    call bios_printStringZ
    ld hl,newline
    call bios_printStringZ
    xor a
    ret
exportWrite:
    ld hl,exportIO
    call fileWriteAll
    jp restoreDrive
; Return HL at basename after the final drive/path separator.
exportLeaf:
    ld d,h
    ld e,l
exportLeafScan:
    ld a,(hl)
    or a
    jp z,exportLeafDone
    inc hl
    cp ':'
    jp z,exportLeafMark
    cp 05Ch
    jp nz,exportLeafScan
exportLeafMark:
    ld d,h
    ld e,l
    jp exportLeafScan
exportLeafDone:
    ex de,hl
    ret
exportDrive:    DB 0
outputPath:     DS MAX_PATH+4,0
exportAuxPath:  DS MAX_PATH+13,0
exportIO:       DW 0,0,0,0
    DS FILE_DESCRIPTOR,0
exportNoRam: DB 'NO WRITABLE RAM DISK: FONT NOT SAVED.',13,10,0
exportSaved: DB 'SAVED: ',0
