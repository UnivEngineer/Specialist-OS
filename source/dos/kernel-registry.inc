; Реестр физических устройств принадлежит DOS, а не сменным службам.
; Запись: blocks, step, capabilities/technology, shift, kind, text[64], reserve[8].
; BIOS A: регистрируется после установки; B: использует общий CPM32.
; H: поступает из одноразового инициализатора, единственная активная копия.
registryBoot:
    ld hl,DOS_DRIVER_REGISTRY
    ld bc,8*DRIVER_RECORD_SIZE
    call cliZero
    ld a,(memoryRamPages)
    ld (DOS_DRIVER_REGISTRY+DRIVER_RECORD_SIZE+1),a
    ld hl,(DISK_TECH_RAM << 8)+DISK_CAP_READ+DISK_CAP_WRITE+DISK_CAP_VERIFY
    ld (DOS_DRIVER_REGISTRY+DRIVER_RECORD_SIZE+4),hl
    ld a,8
    ld (DOS_DRIVER_REGISTRY+DRIVER_RECORD_SIZE+6),a
    ld hl,registryRamName
    ld de,DOS_DRIVER_REGISTRY+DRIVER_RECORD_SIZE+8
    ld bc,registryRamNameEnd-registryRamName
    call moveBytesForward
    IFDEF MX_FLOPPY_BOOT
    ; BOOT устанавливает службу A: и сообщает её фактическую длину.
    ld hl,DOS_FLOPPY_ADDR
    ld (diFloppyStart),hl
    ld hl,(bootFloppyEnd)
    ld (diFloppyEnd),hl
    ld hl,bootFloppyRecord
    ld de,DOS_DRIVER_REGISTRY+FLOPPY_DRIVE*DRIVER_RECORD_SIZE
    ld bc,bootFloppyRecordEnd-bootFloppyRecord
    call moveBytesForward
    xor a
    ret
    ELSE
    ld a,SYSTEM_DRIVE
    ld hl,0
    jp dosDriverRefresh
    ENDIF
    IFDEF MX_FLOPPY_BOOT
bootFloppyEnd: DW 0
bootFloppyRecord:
    DW 1440,0
    DB DISK_CAP_READ+DISK_CAP_WRITE+DISK_CAP_VERIFY,DISK_TECH_FLOPPY,9,DRIVER_KIND_FLOPPY
    DB "MX FDD",0,"VG93 720K DD",0
bootFloppyRecordEnd:
    ENDIF
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
; HL=0 выбирает встроенное описание ROM512KS из DOS. BIOS возвращает только BC.
; CF/A=ошибка, остальные регистры разрушены. Геометрия FAT не меняется.
dosDriverRefresh:
    cp SYSTEM_DRIVE
    jp nz,blockArgument
    ld a,h
    or l
    jp z,registryUseBuiltin
    ld de,KD_PATH
    ld bc,(64 << 8)+1
    call kdReadString
    jp c,blockArgument
    call kdReadString
    jp c,blockArgument
    ld a,e
    sub KD_PATH & 0FFh
    ld hl,KD_PATH
    jp registryTextReady
registryUseBuiltin:
    ld hl,registryRomName
    ld a,registryRomNameEnd-registryRomName
registryTextReady:
    ld (registrySystemText),hl
    ld (registryCopyLength+1),a
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
    ld de,DOS_DRIVER_REGISTRY+SYSTEM_DRIVE*DRIVER_RECORD_SIZE+8
registryCopyLength:
    ld a,0
    call registryCopyText
    IFDEF MX_FLOPPY_BOOT
    ld hl,diskDriver
    ld (v_drives+2*SYSTEM_DRIVE),hl
    ENDIF
    ld a,SYSTEM_DRIVE
    call invalidateDriveCache
    jp dosClearError

    IFDEF MX_FLOPPY_BOOT
registryRomName:        DB "ROM48KS",0,"MX LINEAR ROM",0
    ELSE
registryRomName:        DB "ROM512KS",0,"BUILT-IN, READ ONLY",0
    ENDIF
registryRomNameEnd:
    ASSERT registryRomNameEnd-registryRomName <= 64

; HL — две проверенные строки, DE — поле реестра, A — длина с обоими нулями.
registryCopyText:
    push af
    ld c,a
    ld b,0
    call moveBytesForward
    ld h,b
    ld l,c
    pop af
    cpl
    add a,65
    ret z
    ld c,a
    ld b,0
    jp cliZero

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
    jp z,diVersionReady
    cp DRIVER_FLOPPY_RELOC
    jp nz,blockArgument
    inc hl
    inc hl
    ld a,(hl)
    dec hl
    dec hl
    cp DRIVER_KIND_FLOPPY
    jp nz,blockArgument
diVersionReady:
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
    ld a,8
    jp nz,diCheckShift
    inc a
diCheckShift:
    inc hl
    cp (hl)
    jp nz,blockArgument
    ; Нулевая ёмкость допустима для службы без носителя.
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
    ld de,DOS_BANK_DRIVER_END-DOS_ARENA_START
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
    call registryCopyText
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
    ld hl,(diNewBase)
    ld (diFloppyStart),hl
    ld hl,(diNewEntry)
    ld (diFloppyCall+1),hl
    ld hl,(diNewEnd)
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
; Подготовить копию службы ВГ93 перед публикацией реестра.
; вход: diDescriptor, diImage и diSize проверены по телу активного установщика.
; выход: CF=0 — подготовлены diNewBase/End/Entry, CF=1/A — ошибка. Все регистры рабочие.
; У v2 первые три слова задают размер тела, смещение входа и число WORD16-поправок.
;----------------------------------------------------------------------------
diPrepareFloppy:
    ld hl,(diDescriptor)
    ld a,(hl)
    cp DRIVER_INSTALL_VERSION
    jp z,diFixedFloppy
    ld hl,(diSize)
    ld de,6
    call sub_hl_de
    jp c,blockArgument
    push hl                 ; Остаток пакета: тело и таблица.
    ld hl,(diImage)
    ld c,(hl)
    inc hl
    ld b,(hl)
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    push hl
    ld h,d
    ld l,e
    ld (diNewEntry),hl
    ld h,b
    ld l,c
    ld (diSize),hl
    call cmp_hl_de
    pop hl
    jp c,diInvalidPacket
    jp z,diInvalidPacket
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld (diImage),hl         ; Теперь указывает на тело за заголовком.
    ex de,hl
    ld (diFixCount),hl
    add hl,hl
    jp c,diInvalidPacket
    add hl,bc
    jp c,diInvalidPacket
    pop de
    call cmp_hl_de
    jp nz,blockArgument
    ld hl,0                 ; Встроенная служба WORD16 имеет байтовое выравнивание.
    call relocDosHole
    jp c,dosSetError
    ld (diNewBase),hl
    ld hl,(diNewEntry)
    ex de,hl
    ld hl,(diNewBase)
    add hl,de
    ld (diNewEntry),hl
    call diFloppyBounds
    call dosArenaImage
    ret c
    call diMoveService
    ld hl,(diImage)
    push hl
    ld hl,(diSize)
    ex de,hl
    pop hl
    add hl,de
    ld de,0                 ; Нижняя допустимая позиция следующего слова.
diFixNext:
    push hl
    ld hl,(diFixCount)
    ld a,h
    or l
    jp z,diFixDone
    dec hl
    ld (diFixCount),hl
    pop hl
    ld c,(hl)
    inc hl
    ld b,(hl)
    inc hl
    push hl
    ld h,b
    ld l,c
    call cmp_hl_de
    jp c,diInvalidPacket
    ld de,2
    add hl,de
    jp c,diInvalidPacket
    ex de,hl                ; Конец текущей поправки.
    ld hl,(diSize)
    call cmp_hl_de
    jp c,diInvalidPacket
    ld hl,(diNewBase)
    add hl,bc
    ld c,(hl)
    inc hl
    ld b,(hl)
    push de
    ex de,hl
    ld hl,(diNewBase)
    add hl,bc
    jp c,diFixOverflow
    ld a,h
    ld (de),a
    dec de
    ld a,l
    ld (de),a
    pop de
    pop hl
    jp diFixNext
diFixOverflow:
    pop de
diInvalidPacket:
    pop hl
    jp blockArgument
diFixDone:
    pop hl
    ret
diFixedFloppy:
    ld hl,DOS_FLOPPY_ADDR
    ld (diNewBase),hl
    ld (diNewEntry),hl
    call diFloppyBounds
    ld hl,DOS_BANK_DRIVER_ADDR
    call cmp_hl_de
    jp c,blockArgument
    ; Для прежнего образа по тому же адресу проверяется только прирост.
    ld hl,(diFloppyStart)
    ld bc,DOS_FLOPPY_ADDR
    ld a,h
    cp b
    jp nz,diFixedWhole
    ld a,l
    cp c
    jp nz,diFixedWhole
    ld hl,(diFloppyEnd)
    call cmp_hl_de
    jp nc,diMoveService
    jp diFixedClaim
diFixedWhole:
    ld hl,(diNewBase)
diFixedClaim:
    call dosArenaImage
    ret c
diMoveService:
    ld hl,(diSize)
    ld c,l
    ld b,h
    ld hl,(diNewBase)
    ex de,hl
    ld hl,(diImage)
    call moveBytesForward
    or a
    ret
diFloppyBounds:
    ld hl,(diNewBase)
    push hl
    ld hl,(diSize)
    ex de,hl
    pop hl
    push hl
    add hl,de
    ld (diNewEnd),hl
    ex de,hl
    pop hl
    ret

; Вход: diDescriptor действителен. Выход: a=диск, hl=поле drive, bc/de сохранены.
diDrive:
    ld hl,(diDescriptor)
    inc hl
    ld a,(hl)
    ret
; Вернуть тип службы установщика в A. BC и DE сохранены, HL рабочий.
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
    ld hl,(diBodyStart)
    ex de,hl
    pop hl
    call cmp_hl_de
    jp c,blockArgument
    push hl
    add hl,bc
    jp c,diRangeBad
    ex de,hl
    ld hl,(diBodyEnd)
    call cmp_hl_de
    pop hl
    jp c,blockArgument
    or a
    ret
diRangeBad:
    pop hl
    jp blockArgument

; HL=буфер восьми номеров драйверов, включая службы без носителя.
; B=число, HL=конец; AF/C/DE разрушены.
registryDrives:
    ld de,DOS_DRIVER_REGISTRY+5 ; Технология ненулевая для установленной службы.
    ld bc,0
registryDriveLoop:
    ld a,(de)
    or a
    jp z,registryDriveNext
    ld (hl),c
    inc hl
    inc b
registryDriveNext:
    ex de,hl
    push bc
    ld bc,DRIVER_RECORD_SIZE
    add hl,bc
    pop bc
    ex de,hl
    inc c
    ld a,c
    cp 8
    jp nz,registryDriveLoop
    jp dosClearError
