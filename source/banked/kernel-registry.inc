; Реестр физических устройств принадлежит DOS, а не сменным службам.
; Запись: blocks, step, capabilities/technology, shift, kind, text[64], reserve[8].
; BIOS A: регистрируется после установки; B: использует общий CPM16.
; H: поступает из одноразового инициализатора, единственная активная копия.
registryBoot:
    ld hl,DOS_DRIVER_REGISTRY
    ld bc,8*DRIVER_RECORD_SIZE
    xor a
registryClear:
    ld (hl),a
    inc hl
    dec bc
    ld a,b
    or c
    jp z,registryDefaults
    xor a
    jp registryClear
registryDefaults:
    ld hl,diskDriverDummy
    ld (v_drives+14),hl
    ld a,(bankPageCount)
    dec a
    ld (DOS_DRIVER_REGISTRY+DRIVER_RECORD_SIZE+1),a
    ld hl,(DISK_TECH_RAM << 8)+DISK_CAP_READ+DISK_CAP_WRITE+DISK_CAP_VERIFY
    ld (DOS_DRIVER_REGISTRY+DRIVER_RECORD_SIZE+4),hl
    ld a,8
    ld (DOS_DRIVER_REGISTRY+DRIVER_RECORD_SIZE+6),a
    ld hl,registryRamName
    ld de,DOS_DRIVER_REGISTRY+DRIVER_RECORD_SIZE+8
    ld bc,registryRamNameEnd-registryRamName
    call moveBytesForward
    xor a
    jp dosDriverRefresh
registryRamName: DB "Bank RAM",0,"CPM16; DOS reserved",0
registryRamNameEnd:

; A=диск -> HL=запись; AF/DE разрушены. BC сохранён.
registryRecord:
    ld l,a
    ld h,0
    ld d,h
    ld e,l
    add hl,hl
    add hl,hl
    add hl,de
    add hl,hl
    add hl,hl
    add hl,hl
    add hl,hl
    ld de,DOS_DRIVER_REGISTRY
    add hl,de
    ret
registrySize:
    ld a,(v_drive)
    call registryRecord
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld c,(hl)
    inc hl
    ld b,(hl)
    ld hl,v_diskInfo
    or a
    ret
registryInfo:
    ld a,(v_drive)
    call registryRecord
    ld de,4
    add hl,de
    ld c,(hl)
    inc hl
    ld b,(hl)
    inc hl
    inc hl
    inc hl
    or a
    ret

; A=0: обновить реестр системного носителя после установки BIOS-драйвера.
; CF/A=ошибка, остальные регистры разрушены. Геометрия FAT не меняется.
dosDriverRefresh:
    or a
    jp nz,blockArgument
    ld b,3
    ld hl,kernelSystemInfo
    call dosSystemDriver
    ret c
    ex de,hl
    ld (DOS_DRIVER_REGISTRY),hl
    ld h,b
    ld l,c
    ld (DOS_DRIVER_REGISTRY+2),hl
    ld b,0FFh
    ld hl,kernelSystemInfo
    call dosSystemDriver
    ret c
    ld h,b
    ld l,c
    ld (DOS_DRIVER_REGISTRY+4),hl
    ld a,8
    ld (DOS_DRIVER_REGISTRY+6),a
    ld hl,kernelSystemInfo
    ld de,DOS_DRIVER_REGISTRY+8
    ld bc,64
    call moveBytesForward
    jp dosClearError

; HL=DRIVER_INSTALL в загруженном инициализаторе. Сначала проверяем все
; указатели/размеры и завершаем запись, затем заменяем запись и код.
; CF/A=ошибка, AF/BC/DE/HL разрушены. Вызов разрешён только из DOS.
dosDriverInstall:
    ld a,(diActive)
    or a
    jp z,blockArgument
    ld (diDescriptor),hl
    ld bc,DRIVER_INSTALL
    call diRange
    ret c
    ld a,(hl)
    cp DRIVER_INSTALL_VERSION
    jp nz,blockArgument
    inc hl
    ld a,(hl)
    cp 7
    jp nz,blockArgument
    inc hl
    ld a,(hl)
    cp 3
    jp nc,blockArgument
    inc hl
    ld a,(hl)
    cp 8
    jp nz,blockArgument
    inc hl
    ld a,(hl)
    inc hl
    or (hl)
    jp z,blockArgument
    ld hl,(diDescriptor)
    ld de,DRIVER_INSTALL.image
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld c,(hl)
    inc hl
    ld b,(hl)
    ex de,hl
    ld (diImage),hl
    ld a,b
    or c
    jp z,blockArgument
    push hl
    ld h,b
    ld l,c
    ld de,LOCAL_DRIVER_END-LOCAL_DRIVER_ADDR
    call cmp_hl_de
    pop hl
    jp c,diSizeReady
    jp nz,blockArgument
diSizeReady:
    ld a,b
    ld (diSize+1),a
    ld a,c
    ld (diSize),a
    call diRange
    ret c
    ld hl,(diDescriptor)
    ld de,DRIVER_INSTALL.text
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld (diText),hl
    ld b,64
    ld c,2
diTextScan:
    push bc
    ld bc,1
    call diRange
    pop bc
    ret c
    ld a,(hl)
    inc hl
    or a
    jp nz,diTextNext
    dec c
    jp z,diTextReady
diTextNext:
    dec b
    jp nz,diTextScan
    jp blockArgument
diTextReady:
    ld a,65
    sub b
    ld (diTextSize),a
    call flushVolume
    ret c
    call registryFlushH
    ret c
    ; После этой точки все операции — проверенные копирования внутри DOS.
    ld hl,(diDescriptor)
    ld de,4
    add hl,de
    ld de,DOS_DRIVER_REGISTRY+7*DRIVER_RECORD_SIZE
    ld bc,6
    call moveBytesForward
    ld hl,(diDescriptor)
    inc hl
    inc hl
    ld a,(hl)
    ld (kernelHDriverKind),a
    ld (DOS_DRIVER_REGISTRY+7*DRIVER_RECORD_SIZE+7),a
    inc hl
    ld a,(hl)
    ld (DOS_DRIVER_REGISTRY+7*DRIVER_RECORD_SIZE+6),a
    ld (v_blockShifts+7),a
    ld hl,(diText)
    ld de,DOS_DRIVER_REGISTRY+7*DRIVER_RECORD_SIZE+8
    ld a,(diTextSize)
    ld c,a
    ld b,0
    call moveBytesForward
    ; Остаток строки обнуляем, чтобы реестр не выдавал прежнее описание.
    ld a,64
    ld hl,diTextSize
    sub (hl)
    ld b,a
    or a
    jp z,diCopyCode
    xor a
diClearText:
    ld (de),a
    inc de
    dec b
    jp nz,diClearText
diCopyCode:
    ld hl,(diSize)
    ld (registryImageSize),hl
    ld b,h
    ld c,l
    ld hl,(diImage)
    ld de,DOS_DRIVER_IMAGE
    call moveBytesForward
    ld hl,(registryImageSize)
    ld b,h
    ld c,l
    ld hl,DOS_DRIVER_IMAGE
    ld de,LOCAL_DRIVER_ADDR
    call moveBytesForward
    ld a,0FFh
    ld (klInstalled),a
    xor a
    ld (klFastOwner),a
    ld hl,diskDriver
    ld (v_drives+14),hl
    ld a,(v_drive)
    cp 7
    jp nz,dosClearError
    ld (j_diskDriver+1),hl
    call mountVolume
    ; Установка физического драйвера допустима и для неформатированного тома.
    jp dosClearError

; Дождаться конвейера прежнего H: до опроса/смены драйвера.
registryFlushH:
    ld a,(kernelHDriverKind)
    cp 2
    jp nz,dosClearError
    ld b,4
    jp fastDriver

; Диапазон должен полностью принадлежать текущему инициализатору.
; HL/BC сохранены; AF/DE разрушены, CF/A=ошибка.
diRange:
    ld a,h
    cp DOS_BANK_DRIVER_ADDR/256
    jp c,blockArgument
    push hl
    add hl,bc
    jp c,diRangeBad
    ex de,hl
    ld hl,(diEnd)
    call cmp_hl_de
    pop hl
    jp c,blockArgument
    or a
    ret
diRangeBad:
    pop hl
    jp blockArgument
diActive: DB 0
diEnd: DW 0
diDescriptor: DW 0
diImage: DW 0
diSize: DW 0
diText: DW 0
diTextSize: DB 0
registryImageSize: DW 0

; HL=буфер восьми номеров. B=число, HL=конец; AF/C/DE разрушены.
registryDrives:
    ld de,DOS_DRIVER_REGISTRY
    ld bc,0
registryDriveLoop:
    ld a,(de)
    inc de
    push hl
    ld h,a
    ld a,(de)
    or h
    pop hl
    jp z,registryDriveNext
    ld (hl),c
    inc hl
    inc b
registryDriveNext:
    ex de,hl
    push bc
    ld bc,DRIVER_RECORD_SIZE-1
    add hl,bc
    pop bc
    ex de,hl
    inc c
    ld a,c
    cp 8
    jp nz,registryDriveLoop
    jp dosClearError
