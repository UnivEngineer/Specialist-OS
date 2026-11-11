; Bootstrap уже установил транспорт и образ DOS; повторного чтения DOS нет.
biosBankCold:
    ld a,(biosBootPages)
    ld (bios_upper.ramPageCount),a
    call biosPrintPhysicalRam
    ld hl,2048
    ld (v_diskInfo.totalSectors),hl
    xor a
    ld (v_drive),a
    call sysInstall
    ld hl,08BFFh
    call setMemTop
    ld a,1
    ld hl,BANK_DOS_GATE+6
    call bankCall
    jp c,biosLoadHalt
    ld hl,biosAutoexecPath
    call biosBootExec
    ld c,0
    ld a,1
    ld hl,BANK_DOS_GATE+9
    call bankCall
    jp biosBankShell
; Имя загрузочного сценария выводится до запуска. Ошибка не скрывается,
; но не мешает входу в оболочку (B: может отсутствовать).
biosBootExec:
    push hl
    call bios_printString
    ld c,0Ah
    call bios_printChar
    pop hl
    call bios_fileExec
    ret nc
biosBootCommandFailed:
    ld hl,biosBootError
    call bios_printString
    MXOS_CALL MXOS_DOS_GET_ERROR
    ld c,a
    call bios_printHexByte
    ld c,0Ah
    jp bios_printChar
 ; Только холодный старт: физический объём, включая зарезервированный банк DOS.
; Геометрия и свободный объём B: определяются отдельно драйвером и MEM.
biosPrintPhysicalRam:
    ld hl,biosRamText
    call printString
    ld a,(biosBootPages)
    ld l,a
    ld h,0
    DUP 6
        add hl,hl
    EDUP
    ex de,hl
    ld b,0
    call printDecWord
    ld hl,biosRamBanksText
    call printString
    ld a,(biosBootPages)
    ld e,a
    ld d,0
    ld b,0
    call printDecWord
    ld hl,biosRamEndText
    jp printString
biosRamText: DB 'RAM: ',0
biosRamBanksText: DB ' KiB, ',0
biosRamEndText: DB ' banks',10,0

biosBootError: DB 'BOOT COMMAND ERROR ',0
biosBankWarm:
    ; Bootstrap уже восстановил транспорт и стеки. До AUTOEXEC доступен
    ; встроенный ROM-драйвер A:, без знания файлов внешних драйверов.
    ld a,(biosBootPages)
    ld (bios_upper.ramPageCount),a
    ld hl,2048
    ld (v_diskInfo.totalSectors),hl
    xor a
    ld (v_drive),a
    call sysInstall
    ld c,1
    ld a,1
    ld hl,BANK_DOS_GATE+9
    call bankCall
    ld hl,08BFFh
    call setMemTop
    ; Согласовать реестр DOS со встроенным ROM после восстановления BIOS.
    ; Драйверы повторно устанавливает только явный сценарий пользователя.
    ld hl,0                     ; ROM-SYS: описание по умолчанию из DOS
    xor a
    MXOS_CALL MXOS_DOS_DRIVER_REFRESH
    ld hl,biosAutoexecPath
    call biosBootExec
    ld c,0
    ld a,1
    ld hl,BANK_DOS_GATE+9
    call bankCall
biosBankShell:
    ld a,3
    MXOS_CALL MXOS_DOS_SET_SHELL
    or a
    ld hl,biosNcPath
    jp z,biosBankLaunchShell
    ld hl,biosCliPath
biosBankLaunchShell:
    push hl
    call bios_printString
    ld c,0Ah
    call bios_printChar
    pop hl
    call bios_fileExec
    jp biosBankShell
biosAutoexecPath: DB "A:",05Ch,"AUTOEXEC.BAT",0
biosNcPath: DB "A:",05Ch,"NC.COM",0
biosBootPages: DB 16
biosBootWarm: DB 0
biosBootEntered: DB 0

; Повторный C000h возвращается в DOS, как C800h; bootstrap нужен для Reset.
; Свежая копия BIOS имеет нулевой флаг; native-вариант сохраняет старый вход.
; Вход: нет. Выход: AF разрушены, BC/DE/HL сохранены.
biosCheckReentry:
    ld a,(v_dosBanked)
    or a
    ret z
    ld a,(biosBootEntered)
    or a
    jp nz,biosBankReturn
    inc a
    ld (biosBootEntered),a
    ret

; Секторный ABI основной памяти обслуживает драйвер выбранного тома DOS.
sysDiskDriver:
    push af
    ld a,(v_dosBanked)
    or a
    jp z,sysDiskNative
    ; После подготовки серии регистры уже имеют контракт драйвера.
    ; Не упаковываем их в MX_REGS ради вызова в этой же странице.
    ld a,(MAIN_RAW_LEASE)
    or a
    jp z,sysDiskRequest
    ld a,b
    dec a
    cp 2
    jp nc,sysDiskRequest
    pop af
    jp sysRawBlock
sysDiskRequest:
    pop af
    MXOS_CALL MXOS_DOS_RAW_IO
    ret
sysDiskNative:
    pop af
    jp diskDriver

biosCliPath: DB "A:",05Ch,"CLI.COM",0

; Программный возврат без перезагрузки BIOS/DOS и переустановки драйверов.
; Старые кадры вызовов main -> DOS -> приложение больше не будут возвращены.
; Сначала сбрасываем контекст main, затем пять байт контекста DOS через FAR,
; не затрагивая её код, кэш, драйверы и выбранную оболочку.
; Вход/выход: нет возврата. Все регистры и прежние стеки уничтожаются.
biosBankReturn:
    di
    ; В STD селектор находится по F7FCh, а FFFCh относится к клавиатуре.
    ; В MX это RAM: запись прочитанного байта сохраняет резидентный хвост.
    ld a,(IO_PAGE_RAM-0800h)
    ld (IO_PAGE_RAM-0800h),a
    ld (IO_PAGE_RAM),a
    ld sp,STACK_ADDR
    ld hl,0
    ld (fast6.preBase),hl ; Буфер приложения больше не принадлежит BIOS.
    call biosRecoverCopyTail
    push af
    call sysInstall
    ld a,2
    call glyphCacheService
    xor a
    ld (bankDepth),a
    ld (bankPage),a
    ld hl,BIOS_BANK_STACK_BOTTOM+64
    ld (bankStackFloor),hl
    ld hl,0
    ld (bankOwner),hl
    ld hl,BIOS_BANK_STACK_TOP
    ld (bankServiceSP),hl
    pop af
    or a
    call nz,biosMemWarning
    ld hl,biosResetDosContext
    call bankCopyFar
    jp c,biosLoadHalt
    call bios_reset_sound
    ld c,0
    ld a,1
    ld hl,BANK_DOS_GATE+9
    call bankCall
    ld hl,08BFFh
    call setMemTop
    jp biosBankShell
biosResetDosContext:
    DB 0
    DW biosEmptyDosContext
    DB 1
    DW bankOwner
    DW 5
biosEmptyDosContext:
    DB 0,0,0
    DW DOS_BANK_STACK_TOP
    ASSERT bankHome == bankOwner+1
    ASSERT bankDepth == bankOwner+2
    ASSERT bankServiceSP == bankOwner+3

bios_reset_sound:
    ld a,0B6h
    ld (IO_TIMER_MODE),a
    ld a,076h
    ld (IO_TIMER_MODE),a
    ld a,036h
    ld (IO_TIMER_MODE),a
    ld a,00Ah
    ld (IO_KEYB_MODE),a
    ld hl,(initVars.beepFreq)
    ld (bios_vars.beepFreq),hl
    ret

; В банковой системе BIOS обслуживает только физический A:. B:/H:
; принадлежат DOS; старый BIOS-драйвер B: иначе затёр бы её страницу.
; A сохранён, BC/DE/HL сохранены; CF=1 при недопустимом диске.
biosPhysicalDriveCheck:
    push bc
    ld b,a
    ld a,(v_dosBanked)
    or a
    ld a,b
    pop bc
    jp z,biosPhysicalNative
    or a
    ret z
    scf
    ret
biosPhysicalNative:
    cp 8
    ccf
    ret
