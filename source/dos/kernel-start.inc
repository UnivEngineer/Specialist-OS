; Первичная инициализация банка. Вызывается транспортом, не меняет его SP.
; Возвращает CF/A=код ошибки монтирования A:. AF/BC/DE/HL разрушены.
    ASSERT $ == BANK_DOS_GATE
    jp kernelDispatch
    jp kernelLocalDispatch
dosBoot:
    jp kernelCold
    jp kernelWarm
kernelCold:
    call resetAllSectorCaches
    ld a,1
    ld (v_bootAutoexec),a
    xor a
    ld (fontConfigValid),a
    ld (fontConfigPending),a
    ld (fontConfigRestoring),a
    call memoryInit
    call moduleInit
    call registryBoot
    ret c
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
j_reboot3:
    jp dosBoot
v_dosModuleReady: DB 0
kernelVersion: DB "DOS ",'0'+DOS_VERSION_MAJOR,'.','0'+DOS_VERSION_MINOR,0Dh,0Ah,0

; Тёплый вход после отбрасывания стеков программ. Образ/тома не перезагружаем.
; C=1 перед AUTOEXEC (ошибка без ожидания клавиши), C=0 при входе в оболочку.
kernelWarm:
    ; Dropped stacks also abandon the DOS-bank queue before any physical COPY.
    ld hl,0
    call mx_queueBind
    xor a
    ld (v_batDepth),a
    ld hl,BAT_BUFFER-BAT_FRAME_SIZE
    ld (v_batFrame),hl
    ld a,c
    ld (v_bootAutoexec),a
    call klReclaim
    ret c
    call klInvalidate
    xor a
    ld (kdBusy),a
    ld (kdFarActive),a
    ld (ksDirect),a
    ld (diActive),a
    call moduleWarm
    ld a,(v_bootAutoexec)
    or a
    call nz,fontConfigReset
    call memoryPublishTop
    ld hl,0
    call cliSetObserver
    jp flushVolume
