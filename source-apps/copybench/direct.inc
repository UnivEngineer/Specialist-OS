; Алгоритм из docs/diskbench/ramd-002/direct.inc (31.10.2026).
; Контрольный вариант: прежний прямой обмен, развёртка по 7 байт.
; Вход FFC0h/FFD0h: C=страница, DE=адрес сектора, HL=основная RAM.
; Выход: основная RAM включена, SP сохранён; AF/B/DE/HL/флаги разрушены.
; Первая порция 4 байта, затем 36 порций по 7. Общего буфера данных нет.
driverBufferedStart:
            jp ramdReadFirst
            BLOCK 13,0
            jp ramdWriteFirst
            BLOCK 13,0
driverBufferedEnd:
            ASSERT driverBufferedEnd-driverBufferedStart == 32

; Установку хвоста и окна выполняет автономный стенд.

ramdTailImage:
    DISP 0FF00h
ramdReadLoop:
index = 0
    DUP 7
    IF index == 3
ramdReadFirst:
    ENDIF
            ld a,c
            ld (IO_PAGE_RAMD),a
            ld a,(de)
            ld (IO_PAGE_RAM),a
            ld (hl),a
            inc hl
            inc e
index = index+1
    EDUP
            jp nz,ramdReadLoop
            ret
ramdWriteLoop:
index = 0
    DUP 7
    IF index == 3
ramdWriteFirst:
    ENDIF
            ld b,(hl)
            ld a,c
            ld (IO_PAGE_RAMD),a
            ld a,b
            ld (de),a
            ld (IO_PAGE_RAM),a
            inc hl
            inc e
index = index+1
    EDUP
            jp nz,ramdWriteLoop
            ret
ramdTailEnd:
            ASSERT ramdTailEnd <= NC_STATE_ADDR
    ENT
ramdTailImageEnd:
