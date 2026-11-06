; Дробные миллисекунды поверх timing.inc.
; Требуются timing_printChar (A=символ) и timing_div_hl_de:
; HL/DE -> HL=частное, DE=остаток, как bios_div_hl_de.
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
    call timing_div_hl_de
    ENDIF
    ld (v_msFraction),hl
    call print_number
    ld a,'.'
    call timing_printChar
    ld hl,(v_msFraction)
    ld de,100
    call timing_div_hl_de
    push de
    ld a,l
    add a,'0'
    call timing_printChar
    pop hl
    ld de,10
    call timing_div_hl_de
    push de
    ld a,l
    add a,'0'
    call timing_printChar
    pop hl
    ld a,l
    add a,'0'
    jp timing_printChar
v_msFraction: DW 0
