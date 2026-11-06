; Алгоритм из docs/diskbench/ramd-002/unrolled.inc (31.10.2026).
; Прототип: аккумулятор, развёртка по 13 байт, последняя порция 9 байт.
; Вход FFC0h/FFD0h: C=страница, DE=адрес сектора, HL=буфер основной RAM.
; Выход: SP сохранён, основная RAM включена; AF/BC/DE/HL/флаги разрушены.
; C.7 задаёт направление. Код в хвостах неизменяемый, стек не переключается.
ramdBuffer = 0FFC3h
driverBufferedStart:
            jp ramdBufferedRead
            BLOCK 13,0
            jp ramdBufferedWrite
            BLOCK 13,0
driverBufferedEnd:
            ASSERT driverBufferedEnd-driverBufferedStart == 32

; Установку хвоста и окна выполняет автономный стенд.

ramdBufferedWrite:
            ex de,hl
            ld a,c
            or 080h
            ld c,a
ramdBufferedRead:
            ld b,19
            jp ramdTailLoop

ramdTailImage:
    DISP 0FF00h
ramdTailLoop:
            ld a,c
            or a
            jp m,ramdSourceMain
            ld (IO_PAGE_RAMD),a
ramdSourceMain:
            ld a,b
            or a
            jp z,ramdSourceLast
index = 0
    DUP 4
            ld a,(de)
            ld (ramdBuffer+index),a
            inc de
index = index+1
    EDUP
ramdSourceLast:
    DUP 9
            ld a,(de)
            ld (ramdBuffer+index),a
            inc de
index = index+1
    EDUP
            ld a,c
            or a
            jp m,ramdTargetDisk
            ld (IO_PAGE_RAM),a
            jp ramdTargetCopy
ramdTargetDisk:
            and 07Fh
            ld (IO_PAGE_RAMD),a
ramdTargetCopy:
            ld a,b
            or a
            jp z,ramdTargetLast
index = 0
    DUP 4
            ld a,(ramdBuffer+index)
            ld (hl),a
            inc hl
index = index+1
    EDUP
ramdTargetLast:
    DUP 9
            ld a,(ramdBuffer+index)
            ld (hl),a
            inc hl
index = index+1
    EDUP
            ld (IO_PAGE_RAM),a
            ld a,b
            or a
            ret z
            dec b
            jp ramdTailLoop
ramdTailEnd:
            ASSERT ramdTailEnd <= NC_STATE_ADDR
    ENT
ramdTailImageEnd:
