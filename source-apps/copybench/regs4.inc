; Алгоритм из docs/diskbench/ramd-003/regs4.inc (31.10.2026).
; Четыре байта через BC/DE. HL удерживает основной указатель.
; SP служит вторым указателем. Нет промежуточной копии данных в общей RAM.
; Вход FFC0h/FFD0h: C=страница, DE=сектор, HL=основная RAM.
; Выход: основная RAM включена, SP сохранён; AF/BC/DE/HL/флаги разрушены.
regSelect = 0FFC3h
regDiskSP = regSelect+1
regPage = regSelect+4
regCallback = regSelect+9
regOldSP = 0FFCEh
regMain = 0FFD3h
driverBufferedStart:
            jp regInitRead
            ld sp,0
            ld a,0
            ld (IO_PAGE_RAMD),a
            jp regReadPop
            DW 0
            jp regInitWrite
            BLOCK 13,0
driverBufferedEnd:
            ASSERT driverBufferedEnd-driverBufferedStart == 32

; Установку хвоста и окна выполняет автономный стенд.

regInitRead:
            ld (regMain),hl
            ld hl,0
            add hl,sp
            ld (regOldSP),hl
            ex de,hl
            ld (regDiskSP),hl
            ld hl,(regMain)
            inc hl
            inc hl
            inc hl
            inc hl
            ld a,c
            ld (regPage),a
            ld a,regReadPop & 0FFh
            ld (regCallback),a
            jp regSelect
regInitWrite:
            ld (regMain),hl
            ld hl,0
            add hl,sp
            ld (regOldSP),hl
            ex de,hl
            inc hl
            inc hl
            inc hl
            inc hl
            ld (regDiskSP),hl
            ld hl,(regMain)
            ld a,c
            ld (regPage),a
            ld a,regWritePush & 0FFh
            ld (regCallback),a
            jp regWriteLoad

ramdTailImage:
    DISP 0FF00h
regReadPop:
            pop bc
            pop de
            ld (IO_PAGE_RAM),a
            ld sp,hl
            push de
            push bc
            inc hl
            inc hl
            inc hl
            inc hl
            ld a,(regDiskSP)
            add a,4
            ld (regDiskSP),a
            jp nz,regSelect
regFinish:
            ld hl,(regOldSP)
            ld sp,hl
            ret
regWriteLoad:
            ld sp,hl
            pop bc
            pop de
            inc hl
            inc hl
            inc hl
            inc hl
            jp regSelect
regWritePush:
            push de
            push bc
            ld (IO_PAGE_RAM),a
            ld a,(regDiskSP)
            add a,4
            ld (regDiskSP),a
            jp nz,regWriteLoad
            ; Последняя порция использует конец сектора xx00h следующей страницы.
            ld a,(regDiskSP+1)
            inc a
            ld (regDiskSP+1),a
            ld a,regWriteLast & 0FFh
            ld (regCallback),a
            jp regWriteLoad
regWriteLast:
            push de
            push bc
            ld (IO_PAGE_RAM),a
            jp regFinish
ramdTailEnd:
            ASSERT ramdTailEnd <= NC_STATE_ADDR
    ENT
ramdTailImageEnd:
