; Штатный B: уже обслуживается DOS. Проверяем том, а не заменяем транспорт.
; Выход: CF/A=ошибка, иначе RET; новый том создаёт FORMAT передачей управления.
; AF/BC/DE/HL разрушены. Исходная KOI восстанавливается перед RET/JMP.
    ex de,hl
    call BankArguments
    ret c
    ld a,(bios_upper.koi7)
    push af
    xor a
    call bios_setFontEncoding
    call BankPrepare
    pop bc
    push af
    ld a,b
    call bios_setFontEncoding
    pop af
    ret c
    or a
    ret z
    ; FORMAT загружается по 0000h и затрёт RAMDISK. На её код не возвращаемся:
    ; на стеке остаётся возврат загрузчика DOS, который продолжит AUTOEXEC.
    ld hl,bankFormatCommand
    jp bios_fileExec

BankPrepare:
    ld a,(bios_upper.ramPageCount)
    cp 2
    jp c,InstallFailed
    ld e,0
    call bios_fileGetSetDrive
    ld (bankPreviousDrive),a
    call BankCheckVolume
    push af
    ld a,(bankPreviousDrive)
    call bios_fileSelectDrive
    ; Ошибка исходного (возможно, неразмеченного B:) тома не заменяет
    ; результат проверки; физический выбор восстанавливается самим DOS.
    pop af
    ret

; A=0 — готов, A=1 — требуется FORMAT; CF=1 — нельзя форматировать автоматически.
BankCheckVolume:
    ld a,1
    call bios_fileSelectDrive
    jp c,BankMountError
    ; Повторная проверка нужна и при исходном текущем B: — не доверяем кэшу BPB.
    call bios_mountVolume
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
    call bios_diskDriver
    jp c,BankIoError
    ld hl,(bankBootSector+3)
    ld de,'M'+('X' << 8)
    call bios_cmp_hl_de
    jp z,BankSuspectVolume
    ld hl,(bankBootSector+54)
    ld de,'F'+('A' << 8)
    call bios_cmp_hl_de
    jp z,BankSuspectVolume
    ld a,(bankBootSector+38)
    cp 029h
    jp z,BankSuspectVolume
    ; Правдоподобный размер сектора сохраняем даже при испорченном OEM/типе.
    ld hl,(bankBootSector+11)
    ld de,128
    call bios_cmp_hl_de
    jp z,BankSuspectVolume
    ld de,256
    call bios_cmp_hl_de
    jp z,BankSuspectVolume
    ld de,512
    call bios_cmp_hl_de
    jp z,BankSuspectVolume
    ld hl,bankFormatText
    call bios_printString
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
    call bios_printString
    pop af
    push af
    ld c,a
    call bios_printHexByte
    ld c,0Ah
    call bios_printChar
    pop af
    scf
    ret

bankPreviousDrive: DB 0
bankFormatCommand: DB 'A:',05Ch,'FORMAT.COM B: -R -Y',0
bankFormatText: DB 'RAMDISK: PREPARING B:',0Ah,0
bankErrorText: DB 'RAMDISK: B: UNCHANGED, ERROR ',0
bankBootSector: DS 256,0

; RAMDISK -P n: ограничение B: перед его первоначальным форматированием.
; Готовый том с иной геометрией и выделенные FAR-блоки не изменяются.
BankArguments:
    call bios_argNext
    jp c,BankNoArguments
    ld a,b
    cp 2
    jp nz,BankArgumentError
    ld a,(de)
    cp '-'
    jp nz,BankArgumentError
    inc de
    ld a,(de)
    call bios_upperCase
    cp 'P'
    jp nz,BankArgumentError
    call bios_argNext
    jp c,BankArgumentError
    ld a,b
    cp 3
    jp nc,BankArgumentError
    ld c,0
BankPageDigits:
    ld a,(de)
    sub '0'
    cp 10
    jp nc,BankArgumentError
    push af
    ld a,c
    add a,a
    add a,a
    add a,c
    add a,a
    ld c,a
    pop af
    add a,c
    cp 16
    jp nc,BankArgumentError
    ld c,a
    inc de
    dec b
    jp nz,BankPageDigits
    push bc
    call bios_argNext
    pop bc
    jp nc,BankArgumentError
    ld a,(bios_upper.ramPageCount)
    dec a
    cp c
    jp nc,BankPagesAvailable
    ld c,a
BankPagesAvailable:
    ld b,c
    ld a,MEM_OP_RAMLIMIT
    MXOS_CALL MXOS_DOS_MEMORY
    ret nc
BankArgumentError:
    ld hl,bankArgumentText
    call bios_printString
    scf
    ret
BankNoArguments:
    xor a
    ret
bankArgumentText: DB 'RAMDISK: INVALID -P OR RAM DISK IS IN USE',0Dh,0Ah,0
