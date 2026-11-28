; Bootstrap уже установил транспорт и образ DOS; повторного чтения DOS нет.
biosBootFinish:
    ld hl,biosAutoexecPath
    call biosBootExec
biosBootShellReady:
    ld c,0
    ld a,1
    ld hl,BANK_DOS_GATE+9
    call bankCall
    jp biosBankShell
; Имя загрузочного сценария выводится до запуска. Ошибка не скрывается,
; но не мешает входу в оболочку (B: может отсутствовать).
biosBootExec:
    push hl
    call printStringZ
    ld c,0Ah
    call printChar
    pop hl
    MXOS_CALL_REGS MXOS_DOS_FILE_EXEC
    ret nc
biosBootCommandFailed:
    MXOS_CALL_REGS MXOS_DOS_GET_ERROR
    ld hl,biosBootError
    jp biosReportBootError
biosNcResumeFailed:
    ld hl,biosNcResumeError
; A=error, HL=prefix. Return CF set so the caller can enter recovery.
biosReportBootError:
    push af
    call printStringZ
    pop af
    call printHexByte
    ld c,0Ah
    call printChar
    ld hl,biosBootWaitText
    call printStringZ
biosBootWaitEnter:
    call getch
    cp 0Dh
    jp nz,biosBootWaitEnter
    ld c,0Ah
    call printChar
    scf
    ret
biosBootError:      DB 'BOOT ERROR ',0
biosNcResumeError:  DB 'NC BANK ERROR ',0
biosBootWaitText:   DB 'Press Enter',0

biosBankShell:
    call biosRestoreFont
    call biosRunShell
    jp nc,biosBankShell
    ; A failed preferred shell must never be retried without a prompt.
    ld a,3
    MXOS_CALL_REGS MXOS_DOS_SET_SHELL
    xor 1
    MXOS_CALL_REGS MXOS_DOS_SET_SHELL
    call biosRunShell
    jp nc,biosBankShell
    jp biosRecoveryStart
biosRunShell:
    ld a,3
    MXOS_CALL_REGS MXOS_DOS_SET_SHELL
    or a
    ld hl,biosCliPath
    jp nz,biosTryShell
    ld a,RESIDENT_SHELL_RESUME
    MXOS_CALL_REGS MXOS_DOS_RESIDENT
    ret nc
    or a
    scf
    jp nz,biosNcResumeFailed
    ld a,h
    or l
    ld hl,biosNcPath
    jp nz,biosTryShell
    ld hl,biosNcMainPath
    jp biosTryShell
biosAutoexecPath:   DB "C:",05Ch,"AUTOEXEC.BAT",0
biosNcPath:         DB "C:",05Ch,"DOS",05Ch,"NC.COM",0
biosNcMainPath:     DB "C:",05Ch,"DOS",05Ch,"NC_MN.COM",0
biosCliPath:        DB "C:",05Ch,"DOS",05Ch,"CLI.COM",0
biosBootRomBlocks:   DW 2048
biosBootPages:      DB 16
biosBootWarm:       DB 0
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
    MXOS_CALL_REGS MXOS_DOS_RAW_IO
    ret
sysDiskNative:
    pop af
j_diskDriver:
    jp diskDriver

; Программный возврат без перезагрузки BIOS/DOS и переустановки драйверов.
; Старые кадры вызовов main -> DOS -> приложение больше не будут возвращены.
; Сначала сбрасываем контекст main, затем пять байт контекста DOS через FAR,
; не затрагивая её код, кэш, драйверы и выбранную оболочку.
; Вход/выход: нет возврата. Все регистры и прежние стеки уничтожаются.
biosBankReturn:
    ; The installed renderer and system-owned font resources survive app exit.
    call biosMoveUseLow
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
    ld a,BIOS_PROFILE_DEFAULT
    call biosSetProfile
    pop af
    or a
    call nz,biosMemWarning
    ld hl,biosResetDosContext
    call bankCopyFar
    jp c,biosLoadHalt
    call bios_reset_sound
    jp biosBootShellReady
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
    call ioResetSound
    ld hl,BIOS_BEEP_DURATION_DEFAULT*256+BIOS_BEEP_FREQ_DEFAULT
    ld (mon2_vars.beepFreq),hl
    ret
    ASSERT mon2_vars.beepDuration == mon2_vars.beepFreq+1

; В банковой системе BIOS обслуживает только физический C:. B:/H:
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
    cp SYSTEM_DRIVE
    ret z
    scf
    ret
biosPhysicalNative:
    cp 8
    ccf
    ret
