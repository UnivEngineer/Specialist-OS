; Локальный U32-счётчик BANK COM. Запуск/возврат FILE_EXEC_NATIVE вне интервала.
; Один интервал должен быть меньше периода канала ВИ53 (8,388608 с).
moduleTimerStart:
    ld hl,0
    ld (elapsed),hl
    ld (elapsed+2),hl
    xor a
    call pitTimerRead
    ld (lastTick),hl
    ret
moduleTimerSample:
    xor a
    call pitTimerRead
    ex de,hl
    ld hl,(lastTick)
    ld a,l
    sub e
    ld c,a
    ld a,h
    sbc a,d
    ld b,a
    ex de,hl
    ld (lastTick),hl
    ld hl,(elapsed)
    add hl,bc
    ld (elapsed),hl
    ld hl,(elapsed+2)
    ld bc,0
    jp nc,moduleTimerHigh
    inc bc
moduleTimerHigh:
    add hl,bc
    ld (elapsed+2),hl
    xor a
    ret
moduleTimerResult:
    ld hl,(elapsed+2)
    ex de,hl
    ld hl,(elapsed)
    xor a
    ret
; Настройка сделана управляющей программой до FILE_EXEC_NATIVE.
    DEFINE TIMING_PIT_READ_ONLY
    INCLUDE "../../source/include/timing-pit.inc"
    UNDEFINE TIMING_PIT_READ_ONLY
elapsed: DD 0
lastTick: DW 0
