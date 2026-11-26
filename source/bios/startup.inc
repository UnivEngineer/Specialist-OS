; DOS устанавливает bootstrap, публичных load/release DOS больше нет.
; Приватный отказ оставлен для архивного bootstrap/BOOT.asm.
biosLoadDosImage:
    ld a,DOS_ERR_UNSUPPORTED
    scf
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
    ld hl,initTextVars
    ld de,initTextVarsEnd
    ld bc,bios_upper.charGen
    call memcpy_bc_hl
    ld a,COLOR_BIOS
    ld (IO_COLOR),a
    ; The boot sector cleared the screen; keep the BOOT.SYS splash.
    ; Diagnostics start at the top, above the logo.
    ld hl,8
    ld (bios_vars.cursorY),hl
    ; Bootstrap installs the default self-contained module before first output.
    ld de,0
    call FONT_ENGINE_INIT
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
    jp biosLoadFailure

; Программный возврат отбрасывает контексты приложения и возвращает оболочку.
; Восстановление BIOS из ROM выполняется только аппаратным Reset/bootstrap.
reboot3:
    ld sp,STACK_ADDR
    ld a,(v_dosBanked)
    or a
    jp nz,biosBankReturn
biosLoadFailure:
    ld hl,biosLoadError
    call printString
biosLoadHalt:
    jp biosLoadHalt

biosVersion:        DB "BIOS ",MXOS_VERSION,'.',MXOS_REVISION,'.',VERSION_BUILD,10,0
biosDosVersion:     DB "DOS ",MXOS_VERSION,'.',MXOS_REVISION,'.',VERSION_BUILD,10,0
biosLoadError:      DB "DOS LOAD ERROR",10,0

