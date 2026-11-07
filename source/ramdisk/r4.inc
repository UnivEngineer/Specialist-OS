; R4: прямой перенос четырёх байтов BC/DE через стек, без буфера данных.
; Прямые входы regInitRead/regInitWrite: C=страница, DE=сектор<<8, HL=main.
; Выход: основная RAM и SP восстановлены; AF/BC/DE/HL/флаги разрушены.
; Нереентерабелен, прерывания запрещены контрактом MXOS.
; На сектор 128 смен банка и 256 чтений/записей полезных байтов.
; Порция чтения 163 такта, записи 173; POP/PUSH данных — 2688 тактов/сектор.
; Основной хвост 175 байт: код 77, входы 66, шаблон общего окна 32.
; В дополнительных страницах нужны только первые 77 байт.
regSelect = 0FFC3h
regDiskSP = regSelect+1
regPage = regSelect+4
regCallback = regSelect+9
regOldSP = 0FFCEh
regMain = 0FFD3h
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
ramdBankTailEnd:
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

driverBufferedStart:
            DS 3,0
            ld sp,0
            ld a,0
            ld (IO_PAGE_RAMD),a
            jp regReadPop
                DW 0
            BLOCK 16,0
driverBufferedEnd:
            ASSERT driverBufferedEnd-driverBufferedStart == 32

ramdTailEnd:
    ASSERT ramdBankTailEnd-regReadPop == 77
    ASSERT ramdTailEnd <= NC_STATE_ADDR
    ENT
ramdTailImageEnd:
