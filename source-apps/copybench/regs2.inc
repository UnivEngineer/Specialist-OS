; Алгоритм из docs/diskbench/ramd-003/regs2.inc (31.10.2026).
; Прямой обмен по два байта через B/C; HL и DE остаются указателями.
; Развёрнуты четыре пары, восемь байт между проверками цикла.
; Вход FFC0h/FFD0h: C=страница, DE=сектор, HL=основная RAM.
; Выход: основная RAM включена, SP сохранён; AF/BC/DE/HL/флаги разрушены.
regPage = 0FFD3h
driverBufferedStart:
            jp regInitRead
            BLOCK 13,0
            jp regInitWrite
            BLOCK 13,0
driverBufferedEnd:
            ASSERT driverBufferedEnd-driverBufferedStart == 32

; Установку хвоста и окна выполняет автономный стенд.

regInitRead:
            ld a,c
            ld (regPage),a
            ex de,hl
            jp regReadLoop
regInitWrite:
            ld a,c
            ld (regPage),a
            jp regWriteLoop

ramdTailImage:
    DISP 0FF00h
regReadLoop:
    DUP 4
            ld a,(regPage)
            ld (IO_PAGE_RAMD),a
            ld b,(hl)
            inc l
            ld c,(hl)
            inc l
            ld (IO_PAGE_RAM),a
            ld a,b
            ld (de),a
            inc de
            ld a,c
            ld (de),a
            inc de
    EDUP
            jp nz,regReadLoop
            ret
regWriteLoop:
    DUP 4
            ld b,(hl)
            inc hl
            ld c,(hl)
            inc hl
            ld a,(regPage)
            ld (IO_PAGE_RAMD),a
            ld a,b
            ld (de),a
            inc e
            ld a,c
            ld (de),a
            inc e
            ld (IO_PAGE_RAM),a
    EDUP
            jp nz,regWriteLoop
            ret
ramdTailEnd:
            ASSERT ramdTailEnd <= NC_STATE_ADDR
    ENT
ramdTailImageEnd:
