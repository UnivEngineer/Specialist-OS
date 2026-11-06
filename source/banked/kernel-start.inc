; Первичная инициализация банка. Вызывается транспортом, не меняет его SP.
; Возвращает CF/A=код ошибки монтирования A:. AF/BC/DE/HL разрушены.
    ASSERT $ == BANK_DOS_GATE
    jp kernelDispatch
    jp kernelLocalDispatch
dosBoot:
    jp kernelCold
    jp kernelWarm
kernelCold:
    call resetSectorCache
    ld a,1
    ld (v_bootAutoexec),a
    call flashDosInit
    jp c,kernelRomDriver
    ld hl,Driver
    ld (v_drives+14),hl
    ld hl,v_diskInfo
    ld (FuncSize+4),hl
    ld a,1
    ld (kernelHDriverKind),a
    jp kernelNoFlash
kernelRomDriver:
    ld hl,romDriver
    ld (v_drives+14),hl
    ld hl,v_diskInfo
    ld (romDiskInfoPointer),hl
    xor a
    ld (kernelHDriverKind),a
kernelNoFlash:
    ld hl,kernelVersion
    call bankDosPrintString
    xor a
    call fileChangeDrive
    ret c
    ld a,1
    ld (v_dosModuleReady),a
    xor a
    ret
loadDosModule:
    xor a
    ret
j_reboot3:
    jp dosBoot
v_dosModuleReady: DB 0
kernelVersion: DB "DOS 2.0",0Dh,0Ah,0

; Тёплый вход после отбрасывания стеков программ. Образ/тома не перезагружаем.
kernelWarm:
    xor a
    ld (v_bootAutoexec),a
    ld (kdBusy),a
    ld (kdFarActive),a
    ld (kiReady),a
    ld hl,0
    call cliSetObserver
    jp flushVolume
