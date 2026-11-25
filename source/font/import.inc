; FONT owns files and memory; transient initializer owns font formats.
; 4000h..47FFh flat input; native header before 5000h..57FFh initializer.
parseOption:
    ld hl,(argTail)
nextOption:
    ld a,(hl)
    or a
    ret z
    cp ' '
    jp nz,optionToken
    inc hl
    jp nextOption
optionToken:
    ld de,optionName
    ld b,12
    call readToken
    ret c
    push hl
    ld hl,optionName
    ld de,optFile
    call equalOption
    jp z,gotFile
    ld hl,optionName
    ld de,optImport
    call equalOption
    jp z,gotImport
    ld hl,optionName
    ld de,optOut
    call equalOption
    jp z,gotOut
    pop hl
    jp badImage
gotFile:
    ld a,1
    ld (useFile),a
    pop hl
    jp nextOption
gotImport:
    ld de,importPath
    jp optionValue
gotOut:
    ld de,outputPath
optionValue:
    pop hl
valueSpace:
    ld a,(hl)
    cp ' '
    jp nz,valueStart
    inc hl
    jp valueSpace
valueStart:
    or a
    jp z,badImage
    ld b,MAX_PATH
    call readToken
    ret c
    jp nextOption
readToken:
    ld a,(hl)
    or a
    jp z,tokenEnd
    cp ' '
    jp z,tokenEnd
    cp '/'
    jp nz,tokenByte
    ld a,05Ch
tokenByte:
    ld (de),a
    inc de
    inc hl
    dec b
    jp nz,readToken
    jp badImage
tokenEnd:
    xor a
    ld (de),a
    ret
equalOption:
    ld a,(hl)
    and 0DFh
    ld c,a
    ld a,(de)
    and 0DFh
    cp c
    ret nz
    ld a,(hl)
    or a
    ret z
    inc hl
    inc de
    jp equalOption
; Both path helpers include the terminator.
copyPath:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    or a
    jp nz,copyPath
    ret
copyToHL:
    ex de,hl
    jp copyPath
findExtension:
    ld a,(hl)
    inc hl
    or a
    jp nz,findExtension
    dec hl
    dec hl
    dec hl
    dec hl
    ret
loadPath:
    call fileReadAll
restoreDrive:
    push af
    ld a,(oldDrive)
    MXOS_CALL_REGS MXOS_DOS_FILE_SELECT_DRIVE
    pop af
    ret
loadInitializer:
    ; Descriptor word +46 is a resident-address pointer to the companion
    ; initializer's ASCIZ filename. Resolve it relative to the DLL directory.
    ld hl,path
    ld de,initializerPath
    call copyPath
    ld hl,initializerPath
    ld de,initializerPath
initDirectory:
    ld a,(hl)
    or a
    jp z,initName
    inc hl
    cp 05Ch
    jp nz,initDirectory
    ld d,h
    ld e,l
    jp initDirectory
initName:
    push de
    ld hl,(staging+IMAGE_HEADER_SIZE+46)
    ld de,FONT_ENGINE_BASE
    call bios_cmp_hl_de
    jp c,initBad
    call bios_sub_hl_de
    push hl
    ld de,13
    add hl,de
    ex de,hl
    ld hl,(imageBytes)
    call bios_cmp_hl_de
    pop hl
    jp c,initBad
    ld de,staging+IMAGE_HEADER_SIZE
    add hl,de
    pop de
    ; At most 12 characters, no directory separators in the module name.
    ld b,13
initNameByte:
    ld a,(hl)
    ld (de),a
    or a
    jp z,readInitializer
    cp 05Ch
    jp z,badImage
    inc hl
    inc de
    dec b
    jp nz,initNameByte
    jp badImage
initBad:
    pop de
    jp badImage
readInitializer:
    ld hl,initializerIO
    call loadPath
    ret c
    ld hl,(initializerIO+FILE_PATH_IO.actual)
    ld de,IMAGE_HEADER_SIZE+8
    call bios_cmp_hl_de
    jp c,badImage
    ld hl,(FONT_INIT_ADDR-IMAGE_HEADER_SIZE+6)
    ld de,FONT_INIT_ADDR
    call bios_cmp_hl_de
    jp nz,badImage
    ld hl,(FONT_INIT_ADDR-IMAGE_HEADER_SIZE+8)
    call bios_cmp_hl_de
    jp nz,badImage
    ld hl,(FONT_INIT_ADDR-IMAGE_HEADER_SIZE+10)
    ld de,IMAGE_HEADER_SIZE
    add hl,de
    ex de,hl
    ld hl,(initializerIO+FILE_PATH_IO.actual)
    call bios_cmp_hl_de
    jp nz,badImage
    ld hl,FONT_INIT_ADDR-IMAGE_HEADER_SIZE
    ld de,FONT_INIT_ADDR-IMAGE_HEADER_SIZE+IMAGE_HEADER_CHECKSUM-1
    call bios_calcCS
    ld hl,(FONT_INIT_ADDR-IMAGE_HEADER_SIZE+IMAGE_HEADER_CHECKSUM)
    ld d,b
    ld e,c
    call bios_cmp_hl_de
    jp nz,badImage
    ld hl,(initializerIO+FILE_PATH_IO.actual)
    ld de,FONT_INIT_ADDR-IMAGE_HEADER_SIZE-1
    add hl,de
    ex de,hl
    ld hl,FONT_INIT_ADDR
    call bios_calcCS
    ld hl,(FONT_INIT_ADDR-IMAGE_HEADER_SIZE+12)
    ld d,b
    ld e,c
    call bios_cmp_hl_de
    jp nz,badImage
    ld hl,FONT_INIT_ADDR+3
    ld de,initializerMagic
    ld b,5
initMagic:
    ld a,(de)
    cp (hl)
    jp nz,badImage
    inc hl
    inc de
    dec b
    jp nz,initMagic
    xor a
    ret
importFont:
    ld a,(outputPath)
    or a
    jp z,importOutputChecked
    ld a,(importPath)
    or a
    jp z,badImage
importOutputChecked:
    ld a,(importPath)
    or a
    ret z
    ld a,(useFile)
    or a
    jp nz,badImage ; Imported glyphs require a freshly generated PRE.
    ld hl,importIO
    call loadPath
    ret c
    ld hl,(importIO+FILE_PATH_IO.actual)
    ld de,2048
    call bios_cmp_hl_de
    jp nz,badImage
    ld hl,initContext
    xor a
    call FONT_INIT_ADDR
    ret
optFile:            DB '-FILE',0
optImport:          DB '-IMPORT',0
optOut:             DB '-OUT',0
optionName:         DS 13,0
importPath:         DS MAX_PATH,0
initializerPath:    DS MAX_PATH+13,0
initializerMagic:   DB 'FIN1',1
initContext:        DW staging+IMAGE_HEADER_SIZE,FONT_IMPORT_ADDR,0
initializerIO:      DW initializerPath,FONT_INIT_ADDR-IMAGE_HEADER_SIZE,FONT_INIT_END-FONT_INIT_ADDR+IMAGE_HEADER_SIZE,0
    DS FILE_DESCRIPTOR,0
importIO: DW importPath,FONT_IMPORT_ADDR,2048,0
    DS FILE_DESCRIPTOR,0
