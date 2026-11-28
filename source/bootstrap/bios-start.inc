; Эта часть живёт в BOOT.SYS. До AUTOEXEC код выполняется из временной
; ступени 8000h, затем управление окончательно переходит в резидентный BIOS.
bootBiosStart:
    ld a,3
    call glyphCacheService
    ld (IO_PAGE_RAM),a
    call bios_reset_sound
    ld hl,bootBiosVars
    ld de,bootTextVars
    ld bc,mon2_vars
    call memcpy_bc_hl
    ld hl,bootTextVars
    ld de,bootTextVarsEnd
    ld bc,bios_upper.charGen
    call memcpy_bc_hl
    ld a,COLOR_BIOS
    ld (IO_COLOR),a
    ; Оставляем логотип BOOT.SYS и начинаем диагностику сверху экрана.
    ld hl,8
    ld (mon2_vars.cursorY),hl
    ld a,BIOS_PROFILE_DEFAULT
    call biosSetProfile
    ld de,0
    call FONT_ENGINE_INIT
    ld hl,biosVersion
    call printString
    ld a,(biosBootPages)
    ld (bios_upper.ramPageCount),a
    ld a,(biosBootWarm)
    or a
    call z,bootPrintPhysicalRam
    ld hl,(biosBootRomBlocks)
    ld (v_diskInfo.totalSectors),hl
    ld a,SYSTEM_DRIVE
    ld (v_drive),a
    call sysInstall

    ld a,(biosBootWarm)
    or a
    jp nz,bootBiosWarm
    ld hl,APP_MEMORY_END-1
    call setMemTop
    ld a,1
    ld hl,BANK_DOS_GATE+6
    call bankCall
    jp c,biosLoadHalt
    jp biosBootFinish
bootBiosWarm:
    ld c,1
    ld a,1
    ld hl,BANK_DOS_GATE+9
    call bankCall
    ld hl,bootDosPrefix
    call printString
    ld hl,biosVersionNumber
    call printString
    ld hl,APP_MEMORY_END-1
    call setMemTop
    ; Восстановленный BIOS использует встроенный драйвер, согласуем с DOS.
    ld hl,0
    ld a,SYSTEM_DRIVE
    call mx_callRegs
    DW MXOS_DOS_DRIVER_REFRESH
    jp biosBootFinish

; Физический объём включает зарезервированный банк DOS.
bootPrintPhysicalRam:
    ld hl,bootRamText
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
    ld hl,bootRamBanksText
    call printString
    ld a,(biosBootPages)
    ld e,a
    ld d,0
    ld b,0
    call printDecWord
    ld hl,bootRamEndText
    jp printString
bootDosPrefix:      DB 'DOS ',0
bootRamText:        DB 'RAM: ',0
bootRamBanksText:   DB ' KiB, ',0
bootRamEndText:     DB ' banks',10,0
bootBiosVars:
    INCBIN "../../bin/BIOS.defaults.bin"
bootTextVars = bootBiosVars+MON2_VARIABLES
bootTextVarsEnd:
