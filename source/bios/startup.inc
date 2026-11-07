; BIOS не использует файловую систему. DOS.SYS лежит непрерывно в ROM
; с 8000h, размещение и размер проверяет сборщик системного образа.
; Загрузка сохраняет BC/DE/HL, AF разрушает; CF=ошибка физического чтения.
loadDosModule:
    ld a,(v_dosModuleReady)
    or a
    ret nz
    call biosLoadDosImage
    ret c
    push hl
    ld hl,biosDosVersion
    call printString
    pop hl
    xor a
    ret
biosLoadDosImage:
    push bc
    push de
    push hl
    ld a,(v_drive)
    push af
    xor a
    ld (v_drive),a
    ld hl,2048
    ld (v_diskInfo.totalSectors),hl
    ld de,128
    ld hl,DOS_CODE_START
    ld c,(dosCoreEnd-DOS_CODE_START+255)/256
biosLoadDosBlock:
    ld b,2
    call diskDriver
    jp c,biosLoadDosDone
    inc de
    inc h
    dec c
    jp nz,biosLoadDosBlock
    ld hl,DOS_CODE_START
    ld de,dosCoreEnd-1
    call calcCS
    ld hl,DOS_IMAGE_SUM
    ld d,b
    ld e,c
    call cmp_hl_de
    jp nz,biosLoadDosCorrupt
    ld a,1
    ld (v_dosModuleReady),a
    ld hl,DOS_CODE_START-1
    call setMemTop
    call resetSectorCache
    xor a
    jp biosLoadDosDone
biosLoadDosCorrupt:
    scf
biosLoadDosDone:
    pop bc
    ld a,b
    ld (v_drive),a
    pop hl
    pop de
    pop bc
    ret

; Пока DOS исполняется в main RAM, отдать её файловым клиентам нельзя.
; Исторический вход сохраняет регистры, не снимает защиту memtop.
releaseDosModule:
    xor a
    ret
v_dosModuleReady: DB 0

; C000h: первый вход из bootstrap или программный перезапуск.
reboot1:
    call biosCheckReentry
    ld a,3
    call glyphCacheService
    ld (IO_PAGE_RAM),a
    call bios_reset_sound
    ld hl,initVars
    ld de,initVarsEnd
    ld bc,bios_vars
    call memcpy_bc_hl
    ld a,COLOR_BIOS
    ld (IO_COLOR),a
    call clearScreen
    ; Очистка не меняет координаты: первая строка начинается сверху.
    ld hl,8
    ld (bios_vars.cursorY),hl
    ; Внешний шрифт уже загружен bootstrap до первого вывода.
    ld hl,biosVersion
    call printString
    ld a,(v_dosBanked)
    or a
    jp z,biosNativeCold
    ld a,(biosBootWarm)
    or a
    jp nz,biosBankWarm
    jp biosBankCold
biosNativeCold:
    ld hl,RAMD_PAGE_END
    ld a,(bios_vars.ramPageCount)
    call checkRAMD
    ld a,(v_dosModuleReady)
    or a
    jp z,reboot3
    ld hl,biosDosVersion
    call printString
    jp reboot3

; Программный возврат отбрасывает контексты приложения и возвращает оболочку.
; Восстановление BIOS из ROM выполняется только аппаратным Reset/bootstrap.
reboot3:
    ld sp,STACK_ADDR
    ld a,(v_dosBanked)
    or a
    jp nz,biosBankReturn
    call sysInstall
    call loadDosModule
    jp nc,dosBoot
    ld hl,biosLoadError
    call printString
biosLoadHalt:
    jp biosLoadHalt

biosVersion: DB "BIOS ",'0'+BIOS_VERSION_MAJOR,'.','0'+BIOS_VERSION_MINOR,10,0
biosDosVersion: DB "DOS ",'0'+DOS_VERSION_MAJOR,'.','0'+DOS_VERSION_MINOR,10,0
biosLoadError: DB "DOS load error",10,0

