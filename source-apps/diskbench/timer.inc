; DISKBEN использует аппаратный таймер через опубликованные входы DOS.
timer_init = bios_timerInit
timer_read = bios_timerRead
timing_print = puts
timing_printChar:
    ld c,a
    jp bios_printChar
TIMING_NUMBER_WIDTH = 8
DECIMAL = inputBuffer
    INCLUDE "../../source/include/timing.inc"

; HL=U32 тиков. Печать миллисекунд с тремя знаками после точки.
; ticks_to_ms возвращает частное в v_number и остаток деления в BC.
; 2 МГц: остаток * 8 микросекунд; 2,5 МГц: остаток * 8 / 5.
; Вывод вне измеряемого интервала, все регистры разрушены.
print_time_ms:
    call ticks_to_ms
    ld h,b
    ld l,c
    add hl,hl
    add hl,hl
    add hl,hl
    IF PIT_HZ == 2500000
    ld de,5
    call bios_div_hl_de
    ENDIF
    ld (v_msFraction),hl
    call print_number
    ld a,'.'
    call timing_printChar
    ld hl,(v_msFraction)
    ld de,100
    call bios_div_hl_de
    push de
    ld a,l
    add a,'0'
    call timing_printChar
    pop hl
    ld de,10
    call bios_div_hl_de
    push de
    ld a,l
    add a,'0'
    call timing_printChar
    pop hl
    ld a,l
    add a,'0'
    jp timing_printChar
v_msFraction: DW 0
