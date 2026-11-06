; Алгоритм из docs/diskbench/ramd-002/loop.inc (31.10.2026).
; Прототип: аккумулятор и простой побайтовый цикл, порции 12+...+12+4.
; Вход FFC0h/FFD0h: C=страница, DE=адрес сектора, HL=буфер основной RAM.
; Выход: SP сохранён, основная RAM включена; AF/BC/DE/HL/флаги разрушены.
; Состояние обмена лежит в общем окне; C.7 задаёт направление.
ramdBuffer = 0FFC3h
ramdSource = 0FFD3h
ramdTarget = 0FFD5h
ramdCount = 0FFD7h
ramdLength = 0FFD8h
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
            ld (ramdTarget),hl
            ex de,hl
            ld (ramdSource),hl
            ld a,22
            ld (ramdCount),a
            jp ramdTailLoop

ramdTailImage:
    DISP 0FF00h
ramdTailLoop:
            ld a,(ramdCount)
            cp 1
            ld a,12
            jp nz,ramdLengthReady
            ld a,4
ramdLengthReady:
            ld (ramdLength),a
            ld b,a
            ld hl,(ramdSource)
            ld de,ramdBuffer
            ld a,c
            or a
            jp m,ramdSourceMain
            ld (IO_PAGE_RAMD),a
ramdSourceMain:
            ld a,(hl)
            ld (de),a
            inc hl
            inc de
            dec b
            jp nz,ramdSourceMain
            ld (ramdSource),hl
            ld hl,(ramdTarget)
            ex de,hl
            ld hl,ramdBuffer
            ld a,(ramdLength)
            ld b,a
            ld a,c
            or a
            jp m,ramdTargetDisk
            ld (IO_PAGE_RAM),a
            jp ramdTargetCopy
ramdTargetDisk:
            and 07Fh
            ld (IO_PAGE_RAMD),a
ramdTargetCopy:
            ld a,(hl)
            ld (de),a
            inc hl
            inc de
            dec b
            jp nz,ramdTargetCopy
            ex de,hl
            ld (ramdTarget),hl
            ld (IO_PAGE_RAM),a
            ld a,(ramdCount)
            dec a
            ld (ramdCount),a
            jp nz,ramdTailLoop
            ret
ramdTailEnd:
            ASSERT ramdTailEnd <= NC_STATE_ADDR
    ENT
ramdTailImageEnd:
