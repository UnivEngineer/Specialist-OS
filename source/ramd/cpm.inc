; Секторная адаптация Syabr CP/M 0.1: 16 порций по 16 байт.
; Исходный алгоритм зафиксирован отдельно в copybench/CPMCOPY.asm.
; Одинаковый хвост в main и страницах ДОЗУ. 0FFh обозначает main.
; Всё окно принадлежит драйверу: FFC0h..FFCFh — непрерывный буфер 16 байт.
; FFD0h/FFD2h — адреса, FFD4h — счётчик, FFD6h/FFD7h — страницы, FFD9h — SP.
; Входов кода в окне нет: секторный обработчик вызывает резидент напрямую.
; Выход в main, SP восстановлен; все регистры/флаги разрушены.
ramdTailImage:
    DISP 0FF00h
cpmCopySector:
    ld a,(0FFD6h)
    cp 0FFh
    jp z,cpmSourceMain
    ld (IO_PAGE_RAMD),a
    jp cpmRead
cpmSourceMain:
    ld (IO_PAGE_RAM),a
cpmRead:
    ld hl,(0FFD0h)
    ld sp,hl
    ld de,16
    add hl,de
    ld (0FFD0h),hl
index = 0
    DUP 8
    pop hl
    ld (0FFC0h+index*2),hl
index = index+1
    EDUP
    ld a,(0FFD7h)
    cp 0FFh
    jp z,cpmDestinationMain
    ld (IO_PAGE_RAMD),a
    jp cpmWrite
cpmDestinationMain:
    ld (IO_PAGE_RAM),a
cpmWrite:
    ld hl,(0FFD2h)
    ld de,16
    add hl,de
    ld (0FFD2h),hl
    ld sp,hl
index = 0
    DUP 8
    ld hl,(0FFCEh-index*2)
    push hl
index = index+1
    EDUP
    ld hl,0FFD4h
    dec (hl)
    jp nz,cpmCopySector
    ld (IO_PAGE_RAM),a
    ld hl,(0FFD9h)
    ld sp,hl
    ret
ramdBankTailEnd:
driverBufferedStart:
    ; RESTORE/USER устанавливают начальное состояние рабочего окна.
    DS 32,0
driverBufferedEnd:
    ASSERT driverBufferedEnd-driverBufferedStart == 32
ramdTailEnd:
    ASSERT ramdTailEnd <= NC_STATE_ADDR
    ENT
ramdTailImageEnd:
