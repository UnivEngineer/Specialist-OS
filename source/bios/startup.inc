; DOS устанавливает bootstrap, публичных load/release DOS больше нет.
; Приватный отказ оставлен для архивного bootstrap/BOOT.asm.
biosLoadDosImage:
    ld a,DOS_ERR_UNSUPPORTED
    scf
    ret
v_dosModuleReady: DB 0

; C000h: первый вход из bootstrap или программный перезапуск.
reboot1:
    ld hl,(biosBootEntry)
    ld a,h
    or a
    jp z,reboot3
    xor a
    ld (biosBootEntry+1),a
    jp (hl)
biosBootEntry: DW 0

reboot3:
    di
    ; В STD F7FCh возвращает отображение MX. В MX сохраняем байт RAM по F7FCh.
    ld a,(IO_PAGE_RAM-0800h)
    ld (IO_PAGE_RAM-0800h),a
    ld (IO_PAGE_RAM),a
    ld sp,STACK_ADDR
    ld a,(v_dosBanked)
    or a
    jp nz,biosBankReturn
biosLoadFailure:
    ld hl,biosLoadError
    call printString
biosLoadHalt:
    jp biosLoadHalt

biosVersion:        DB "BIOS "
biosVersionNumber:  DB MXOS_VERSION,'.',MXOS_REVISION,'.',VERSION_BUILD,10,0
biosLoadError:      DB "DOS LOAD ERROR",10,0

