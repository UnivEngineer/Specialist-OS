; Прямой RAW основной памяти; разрешение выдаёт DOS на первую операцию.
sysRawRoute:
    ld a,c
    cp MXOS_DOS_GET_ERROR & 0FFh
    jp nz,sysRawCheck
    ld a,(MAIN_RAW_LEASE+5)
    or a
    jp z,sysRawInvalidate
    ld hl,sysRawError
    jp sysRawDispatch
sysRawCheck:
    cp MXOS_DOS_RAW_IO & 0FFh
    jp nz,sysRawInvalidate
    ld a,(MAIN_RAW_LEASE)
    or a
    jp z,sysRawInvalidate
    ld hl,5
    add hl,de
    ld a,(hl)
    dec a
    cp 2
    jp nc,sysRawInvalidate
    ld hl,sysRawBlock
sysRawDispatch:
    push de
    ld bc,sysReturned
    push bc
    push hl
    jp sysLoadRegs
sysRawInvalidate:
    ; Любой вход в DOS прекращает прямую серию. FLASHFST завершает запись
    ; при реальной передаче устройства, в fastDriver/klInstall.
    xor a
    ld (MAIN_RAW_LEASE),a
    ld (MAIN_RAW_LEASE+5),a
    jp sysBankDoCall

; Локальный RAW сохраняет BC/DE/HL и проверяет границы перед обменом.
; Права на прямую серию и ёмкость выданы DOS первым секторным запросом.
sysRawBlock:
    push bc
    push de
    push hl
    ld a,h
    cp 0F7h
    jp c,sysRawBufferOk
    jp nz,sysRawBad
    ld a,l
    or a
    jp nz,sysRawBad
sysRawBufferOk:
    ld hl,(MAIN_RAW_LEASE+1)
    ld a,d
    cp h
    jp c,sysRawSectorOk
    jp nz,sysRawBad
    ld a,e
    cp l
    jp nc,sysRawBad
sysRawSectorOk:
    ld a,(MAIN_RAW_LEASE)
    dec a
    ld c,a
    cp 1
    jp nz,sysRawInvoke
    ld a,e
    cp 0FFh
    jp z,sysRawBad
sysRawInvoke:
    pop hl
    push hl
    call LOCAL_BLOCK_ENTRY
    jp nc,sysRawSuccess
    ; Аппаратные службы возвращают CF; единый код DOS выдаёт граница ABI.
    ld a,DOS_ERR_IO
    scf
    jp sysRawResult
sysRawBad:
    ld a,DOS_ERR_ARGUMENT
    scf
    jp sysRawResult
sysRawSuccess:
    xor a
sysRawResult:
    push af
    pop hl
    ld (MAIN_RAW_LEASE+3),hl
    ld a,1
    ld (MAIN_RAW_LEASE+5),a
    pop hl
    pop de
    pop bc
sysRawError:
    push hl
    ld hl,(MAIN_RAW_LEASE+3)
    push hl
    pop af
    pop hl
    ret

; Локальные поля общего MEM.SYS; публичный код уже установлен bootstrap.
; Вызывается при холодном старте и тёплом восстановлении MXOS.
sysInstall:
    call ioInitialize
    xor a
    ld (MAIN_RAW_LEASE),a
    ld (MAIN_RAW_LEASE+5),a
    ld hl,sysCall
    ld (MEM_NATIVE_CALL),hl
    ld hl,bios_memcpy_bc_hl
    ld (MEM_NEAR_COPY),hl
    ld hl,bios_blockIO
    ld (MEM_LOCAL_A),hl
    ld hl,localBlockChecked
    ld (MEM_LOCAL_CHECKED),hl
    ld a,MEM_BANK_EXEC
    ld (MEM_BANK_KIND),a
    ret
sysRawEnd:
