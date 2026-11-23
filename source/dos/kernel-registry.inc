; Реестр физических устройств принадлежит DOS, а не сменным службам.
; Запись: blocks, step, capabilities/technology, shift, kind, text[64], reserve[8].
; BIOS A: регистрируется после установки; B: использует общий CPM32.
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
    ld a,7
    call invalidateDriveCache
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
    ld hl,0
    ld a,SYSTEM_DRIVE
    jp dosDriverRefresh
registryRamName: DB "BANK RAM",0,"CPM32; DOS RESERVED",0
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

; A=0: обновить реестр A:. HL=две ASCIIZ клиента, всего <=64 байт;
; HL=0 выбирает встроенное описание ROM-SYS из DOS. BIOS возвращает только BC.
; CF/A=ошибка, остальные регистры разрушены. Геометрия FAT не меняется.
dosDriverRefresh:
    cp SYSTEM_DRIVE
    jp nz,blockArgument
    ld a,h
    or l
    jp z,registryUseBuiltin
    ld de,KD_PATH
    ld bc,64
    call kdFetch
    ret c
    ld hl,KD_PATH
    jp registryValidateText
registryUseBuiltin:
    ld hl,registryRomName
registryValidateText:
    ld (registrySystemText),hl
    ld b,64
    ld c,2
registryValidateByte:
    ld a,(hl)
    inc hl
    or a
    jp nz,registryValidateNext
    dec c
    jp z,registryValidateReady
registryValidateNext:
    dec b
    jp nz,registryValidateByte
    jp blockArgument
registryValidateReady:
    ld b,3
    ld hl,0
    call dosSystemDriver
    ret c
    ex de,hl
    ld (DOS_DRIVER_REGISTRY+SYSTEM_DRIVE*DRIVER_RECORD_SIZE),hl
    ld h,b
    ld l,c
    ld (DOS_DRIVER_REGISTRY+SYSTEM_DRIVE*DRIVER_RECORD_SIZE+2),hl
    ld b,0FFh
    ld hl,0
    call dosSystemDriver
    ret c
    ld h,b
    ld l,c
    ld (DOS_DRIVER_REGISTRY+SYSTEM_DRIVE*DRIVER_RECORD_SIZE+4),hl
    ld a,8
    ld (DOS_DRIVER_REGISTRY+SYSTEM_DRIVE*DRIVER_RECORD_SIZE+6),a
    ld hl,(registrySystemText)
registrySystemCopy:
    ; Две строки ограничены полем реестра. Остаток не содержит кода установщика.
    ld de,DOS_DRIVER_REGISTRY+SYSTEM_DRIVE*DRIVER_RECORD_SIZE+8
    ld b,64
    ld c,2
registrySystemCopyByte:
    ld a,(hl)
    inc hl
    ld (de),a
    inc de
    or a
    jp nz,registrySystemCopyNext
    dec c
    jp z,registrySystemClear
registrySystemCopyNext:
    dec b
    jp nz,registrySystemCopyByte
    jp blockArgument
registrySystemClear:
    dec b
    jp z,registrySystemTextReady
    xor a
registrySystemClearByte:
    ld (de),a
    inc de
    dec b
    jp nz,registrySystemClearByte
registrySystemTextReady:
    ld a,SYSTEM_DRIVE
    call invalidateDriveCache
    jp dosClearError

registryRomName:        DB "ROM-SYS",0,"BUILT-IN, READ ONLY",0

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
    cp 8
    jp nc,blockArgument
    cp SYSTEM_DRIVE
    jp z,blockArgument
    cp RAM_DRIVE
    jp z,blockArgument
    ld de,kernelRamDrive
    ld c,a
    ld a,(de)
    cp c
    jp z,blockArgument
    ld a,c
    ld c,a
    inc hl
    ld a,(hl)
    ld de,kernelExternalDrive
    cp DRIVER_KIND_FLOPPY
    jp c,diCheckLetter
    jp nz,blockArgument
    dec de
diCheckLetter:
    ; A backend occupies one resident slot. Reject a second letter or collision.
    ld a,(de)
    cp c
    jp z,diCheckBlockShift
    inc a
    jp nz,blockArgument
    ld a,(kernelFloppyDrive)
    cp c
    jp z,blockArgument
    ld a,(kernelExternalDrive)
    cp c
    jp z,blockArgument
diCheckBlockShift:
    ld a,(hl)
    cp DRIVER_KIND_FLOPPY
    inc hl
    ld a,(hl)
    jp z,diCheckFloppyShift
    cp 8
    jp diCheckShift
diCheckFloppyShift:
    cp 9
diCheckShift:
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
    call diKind
    cp DRIVER_KIND_FLOPPY
    ld h,b
    ld l,c
    ld de,LOCAL_DRIVER_END-LOCAL_DRIVER_ADDR
    jp nz,diSizeLimit
    ld de,DOS_BANK_DRIVER_ADDR-DOS_FLOPPY_ADDR
diSizeLimit:
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
    call diKind
    cp DRIVER_KIND_FLOPPY
    jp nz,diPublishRegistry
    call diPrepareFloppy
    ret c
diPublishRegistry:
    ; Код, геометрия и обе строки проверены до замены записи.
    call diDrive
    call registryRecord
    ex de,hl
    ld hl,(diDescriptor)
    ld bc,DRIVER_INSTALL.blocks
    add hl,bc
    ld bc,6
    call moveBytesForward
    ld d,b                   ; BC = конец приёмника moveBytesForward.
    ld e,c
    ld hl,(diDescriptor)
    inc hl
    inc hl
    ld b,(hl)               ; kind
    inc hl
    ld a,(hl)               ; blockShift
    ld (de),a
    inc de
    ld a,b
    ld (de),a
    inc de
    ld hl,(diText)
    ld a,(diTextSize)
    ld c,a
    ld b,0
    call moveBytesForward
    ld d,b
    ld e,c
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
    call diKind
    cp DRIVER_KIND_FLOPPY
    jp z,diCopyFloppy
    ld (kernelHDriverKind),a
    call diDrive
    ld (kernelExternalDrive),a
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
    call klInvalidate
    ld a,DOS_IO_PAGE
    ld (klFastOwner),a
    ld c,8
diPublish:
    call diDrive
    ld e,a
    ld d,0
    ld hl,v_blockShifts
    add hl,de
    ld (hl),c
    ld hl,v_drives
    add hl,de
    add hl,de
    ld (hl),diskDriver & 0FFh
    inc hl
    ld (hl),diskDriver >> 8
    ld hl,diskDriver
    call invalidateDriveCache
    ld b,a
    ld a,(v_drive)
    cp b
    jp nz,dosClearError
    ld (j_diskDriver+1),hl
    call mountVolume
    ; Установка физического драйвера допустима и для неформатированного тома.
    jp dosClearError

diCopyFloppy:
    ld hl,(diSize)
    ld b,h
    ld c,l
    ld hl,(diImage)
    ld de,DOS_FLOPPY_ADDR
    call moveBytesForward
    ld h,b
    ld l,c
    ld (diFloppyEnd),hl
    call diDrive
    ld (kernelFloppyDrive),a
    ld e,a
    ld d,0
    ld hl,v_directories
    add hl,de
    add hl,de
    ld (hl),0
    inc hl
    ld (hl),0
    ld c,9
    jp diPublish

;----------------------------------------------------------------------------
; MXOS
;
; Зарезервировать место для физического драйвера в арене DOS.
; вход: diSize — проверенный размер тела. DOS выполняет установщик.
; выход: cf/a — статус. При повторной установке проверяется только прирост.
;   bc, de, hl рабочие. Старая служба остаётся действующей до diCopyFloppy.
;----------------------------------------------------------------------------
diPrepareFloppy:
    ld hl,(diSize)
    ld de,DOS_FLOPPY_ADDR
    add hl,de
    ex de,hl
    ld hl,(diFloppyEnd)
    ld a,h
    or l
    jp nz,diGrowFloppy
    ld hl,DOS_FLOPPY_ADDR
diGrowFloppy:
    call cmp_hl_de
    jp c,dosArenaImage
    ret

; Вход: diDescriptor действителен. Выход: a=диск, hl=поле drive, bc/de сохранены.
diDrive:
    ld hl,(diDescriptor)
    inc hl
    ld a,(hl)
    ret
; Два резервных байта записи D: содержат исключающий конец службы в арене.
; Return the installer kind in A. BC and DE are preserved, HL is scratch.
diKind:
    call diDrive
    inc hl
    ld a,(hl)
    ret

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
    push hl
    ld hl,(diStart)
    ex de,hl
    pop hl
    call cmp_hl_de
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
