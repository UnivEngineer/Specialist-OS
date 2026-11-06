; Алгоритм из docs/diskbench/ramd-003/regs6.inc (31.10.2026).
; Шесть байтов через BC/DE/HL. A остаётся номером страницы.
; Первая порция четыре байта, затем 42 по шесть, без промежуточного буфера.
; Вход FFC0h/FFD0h: C=страница, DE=сектор, HL=основная RAM.
; Выход: основная RAM включена, SP сохранён; AF/BC/DE/HL/флаги разрушены.
regSource = 0FFC3h
regSourceSP = regSource+1
regPage = regSource+4
regSourcePort = regSource+6
regSourceCallback = regSource+9
regOldSP = 0FFCEh
regTarget = 0FFD3h
regTargetSP = regTarget+1
regTargetPort = regTarget+4
regTargetCallback = regTarget+7
regCount = 0FFDCh
driverBufferedStart:
            jp regInitRead
            ld sp,0
            ld a,0
            ld (IO_PAGE_RAMD),a
            jp regPop4
            DW 0
            jp regInitWrite
            ld sp,0
            ld (IO_PAGE_RAM),a
            jp regPush4
            DB 0
            BLOCK 3,0
driverBufferedEnd:
            ASSERT driverBufferedEnd-driverBufferedStart == 32

; Установку хвоста и окна выполняет автономный стенд.

regInitRead:
            ex de,hl
            ld a,0FDh
            jp regInit
regInitWrite:
            ld a,0FCh
regInit:
            ld (regSourcePort),a
            xor 1
            ld (regTargetPort),a
            ld (regSourceSP),hl
            ex de,hl
            inc hl
            inc hl
            inc hl
            inc hl
            ld (regTargetSP),hl
            ld a,c
            ld (regPage),a
            ld hl,0
            add hl,sp
            ld (regOldSP),hl
            ld a,regPop4 & 0FFh
            ld (regSourceCallback),a
            ld a,regPush4 & 0FFh
            ld (regTargetCallback),a
            jp regSource

; Вся подготовка выполняется в основной RAM без CALL и обычного стека.
regPrepare6:
            ld hl,(regSourceSP)
            ld de,4
            add hl,de
            ld (regSourceSP),hl
            ld hl,(regTargetSP)
            ld de,6
            add hl,de
            ld (regTargetSP),hl
            ld a,regPop6 & 0FFh
            ld (regSourceCallback),a
            ld a,regPush6 & 0FFh
            ld (regTargetCallback),a
            ld a,42
            ld (regCount),a
            jp regSource
regNext6:
            ld hl,regCount
            dec (hl)
            jp z,regFinish
            ld hl,(regSourceSP)
            ld de,6
            add hl,de
            ld (regSourceSP),hl
            ld hl,(regTargetSP)
            add hl,de
            ld (regTargetSP),hl
            jp regSource
regFinish:
            ld hl,(regOldSP)
            ld sp,hl
            ret

ramdTailImage:
    DISP 0FF00h
regPop4:
            pop bc
            pop de
            jp regTarget
regPush4:
            push de
            push bc
            ld (IO_PAGE_RAM),a
            jp regPrepare6
regPop6:
            pop bc
            pop de
            pop hl
            jp regTarget
regPush6:
            push hl
            push de
            push bc
            ld (IO_PAGE_RAM),a
            jp regNext6
ramdTailEnd:
            ASSERT ramdTailEnd <= NC_STATE_ADDR
    ENT
ramdTailImageEnd:
