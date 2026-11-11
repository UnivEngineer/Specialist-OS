; Инициализатор исполняется в DOS; B: использует общие службы MEM/CPM16.
; Пул банков и геометрию реестра назначает RAMLIMIT; том создаёт FORMAT.
; AF/BC/DE/HL разрушены. Режим KOI и выбор диска восстанавливаются.
    ld hl,bios_upper.koi7
    ld b,0
    ld a,MEM_OP_GET8
    call mx_memory
    ld a,e
    push af
    xor a
    MXOS_CALL MXOS_BIOS_SET_FONT_ENCODING
    call BankPrepare
    pop bc
    push af
    ld a,b
    MXOS_CALL MXOS_BIOS_SET_FONT_ENCODING
    pop af
    ret c
    or a
    ret z
    ; FORMAT работает в main; наш код C000h в DOS остаётся доступен для RET.
    ld hl,bankFormatCommand
    MXOS_CALL MXOS_DOS_FILE_EXEC_NATIVE
    ret

BankPrepare:
    call mx_memInfo
    ld a,b
    cp 2
    jp c,InstallFailed
    ld e,0
    MXOS_CALL MXOS_DOS_FILE_GET_SET_DRIVE
    ld (bankPreviousDrive),a
    call BankCheckVolume
    push af
    ld a,(bankPreviousDrive)
    MXOS_CALL MXOS_DOS_FILE_SELECT_DRIVE
    ; Ошибка исходного (возможно, неразмеченного B:) тома не заменяет
    ; результат проверки; физический выбор восстанавливается самим DOS.
    pop af
    ret

; A=0 — готов, A=1 — требуется FORMAT; CF=1 — нельзя форматировать автоматически.
BankCheckVolume:
    ld a,1
    MXOS_CALL MXOS_DOS_FILE_SELECT_DRIVE
    jp c,BankMountError
    ; Повторная проверка нужна и при исходном текущем B: — не доверяем кэшу BPB.
    MXOS_CALL MXOS_DOS_MOUNT_VOLUME
    jp c,BankMountError
    xor a
    ret
BankMountError:
    MXOS_CALL MXOS_DOS_GET_ERROR
    cp DOS_ERR_UNSUPPORTED
    jp nz,BankReportError
    ; Неизвестная разметка: читаем boot отдельно. Ошибка чтения не означает,
    ; что носитель пуст. Узнаваемый MXOS/FAT-заголовок тоже не стираем.
    ld b,2
    ld de,0
    ld hl,bankBootSector
    MXOS_CALL MXOS_DOS_RAW_IO
    jp c,BankIoError
    ld hl,(bankBootSector+3)
    ld de,'M'+('X' << 8)
    call cmp_hl_de
    jp z,BankSuspectVolume
    ld hl,(bankBootSector+54)
    ld de,'F'+('A' << 8)
    call cmp_hl_de
    jp z,BankSuspectVolume
    ld a,(bankBootSector+38)
    cp 029h
    jp z,BankSuspectVolume
    ; Правдоподобный размер сектора сохраняем даже при испорченном OEM/типе.
    ld hl,(bankBootSector+11)
    ld de,128
    call cmp_hl_de
    jp z,BankSuspectVolume
    ld de,256
    call cmp_hl_de
    jp z,BankSuspectVolume
    ld de,512
    call cmp_hl_de
    jp z,BankSuspectVolume
    ld hl,bankFormatText
    MXOS_CALL MXOS_BIOS_PRINT_STRING
    ld a,1
    or a
    ret
BankIoError:
    ld a,DOS_ERR_IO
    jp BankReportError
BankSuspectVolume:
    ld a,DOS_ERR_CORRUPT
BankReportError:
    push af
    ld hl,bankErrorText
    MXOS_CALL MXOS_BIOS_PRINT_STRING
    pop af
    push af
    ld c,a
    MXOS_CALL MXOS_BIOS_PRINT_HEX_BYTE
    ld c,0Ah
    MXOS_CALL MXOS_BIOS_PRINT_CHAR
    pop af
    scf
    ret

bankPreviousDrive: DB 0
bankFormatCommand: DB 'A:',05Ch,'FORMAT.COM B: -R -Y',0
bankFormatText: DB 'RAMDISK: PREPARING B:',0Ah,0
bankErrorText: DB 'RAMDISK: B: UNCHANGED, ERROR ',0
bankBootSector: DS 256,0

