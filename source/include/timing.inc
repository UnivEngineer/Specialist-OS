    INCLUDE "timing-counter.inc"

; Пересчёт и десятичный вывод перенесены из LLM80 без связи с его страницей.
; Требуются PIT_HZ (CLK1), DECIMAL (11 байт), timing_print (HL=строка).
; При TIMING_NUMBER_WIDTH>0 также timing_printChar (A=символ).
; Вся рабочая память локальна для экземпляра библиотеки в приложении.
    IF PIT_HZ == 2000000
PIT_SCALE_SHIFT = 4
PIT_MS_DIVISOR = 125
    ELSE
    ASSERT PIT_HZ == 2500000
PIT_SCALE_SHIFT = 6
PIT_MS_DIVISOR = 625
    ENDIF
TIMING_MAX_TICKS = 0FFFFFFFFh >> PIT_SCALE_SHIFT
    IFNDEF TIMING_NUMBER_WIDTH
TIMING_NUMBER_WIDTH = 0
    ENDIF
    ASSERT TIMING_NUMBER_WIDTH >= 0 && TIMING_NUMBER_WIDTH <= 10

; Вход HL/DE=два U32, результат [HL]-=[DE], CF=заём. Все регистры разрушены.
subtract_u32:
    ld c,4
    or a
subtract_byte:
    ld a,(de)
    ld b,a
    ld a,(hl)
    sbc a,b
    ld (hl),a
    inc hl
    inc de
    dec c
    jp nz,subtract_byte
    ret

; Тики -> целые мс с округлением вниз, для PIT_HZ вызывающей программы.
; Вход HL=U32 тики <= TIMING_MAX_TICKS; v_number=U32 мс. Все регистры и флаги разрушены.
ticks_to_ms:
    ld de,v_number
    ld c,4
ms_copy:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec c
    jp nz,ms_copy
    ld b,PIT_SCALE_SHIFT
ms_scale:
    push bc
    ld hl,v_number
    ld c,4
    or a
ms_scale_byte:
    ld a,(hl)
    rla
    ld (hl),a
    inc hl
    dec c
    jp nz,ms_scale_byte
    pop bc
    dec b
    jp nz,ms_scale
    ld a,32
    ld (v_div_bits),a
    ld bc,0
ms_divide:
    ld hl,v_number
    ld d,4
    or a
ms_div_shift:
    ld a,(hl)
    rla
    ld (hl),a
    inc hl
    dec d
    jp nz,ms_div_shift
    ld a,c
    rla
    ld c,a
    ld a,b
    rla
    ld b,a
    ld a,c
    sub LOW PIT_MS_DIVISOR
    ld e,a
    ld a,b
    sbc a,HIGH PIT_MS_DIVISOR
    jp c,ms_div_skip
    ld b,a
    ld c,e
    ld hl,v_number
    inc (hl)
ms_div_skip:
    ld hl,v_div_bits
    dec (hl)
    jp nz,ms_divide
    ret
; Вход v_number=U32; десятичная печать без ведущих нулей. Всё разрушено.
print_number:
    ld hl,powers10
    ld (v_power),hl
    ld hl,DECIMAL
    ld (v_decimal),hl
    ld a,10
    ld (v_digits),a
decimal_power:
    ld a,'0'-1
    ld (v_digit),a
decimal_try:
    ld hl,v_digit
    inc (hl)
    LOAD_DE v_power
    ld hl,v_number
    call subtract_u32
    jp nc,decimal_try
    LOAD_DE v_power
    ld hl,v_number
    ld c,4
    or a
decimal_restore:
    ld a,(de)
    ld b,a
    ld a,(hl)
    adc a,b
    ld (hl),a
    inc hl
    inc de
    dec c
    jp nz,decimal_restore
    ex de,hl
    ld (v_power),hl
    ld hl,(v_decimal)
    ld a,(v_digit)
    ld (hl),a
    inc hl
    ld (v_decimal),hl
    ld hl,v_digits
    dec (hl)
    jp nz,decimal_power
    ld hl,(v_decimal)
    ld (hl),0
    ld hl,DECIMAL
decimal_trim:
    ld a,(hl)
    cp '0'
    jp nz,decimal_print
    inc hl
    ld a,(hl)
    or a
    jp nz,decimal_trim
    dec hl
decimal_print:
    IF TIMING_NUMBER_WIDTH > 0
    ; Длина <= 10: разность младших байтов работает и через границу страницы.
    ld a,(v_decimal)
    sub l
    ld b,a
    ld a,TIMING_NUMBER_WIDTH
    sub b
    jp c,decimal_emit
    jp z,decimal_emit
    ld b,a
decimal_pad:
    push hl
    push bc
    ld a,' '
    call timing_printChar
    pop bc
    pop hl
    dec b
    jp nz,decimal_pad
    ENDIF
decimal_emit:
    jp timing_print
powers10:        DD 1000000000,100000000,10000000,1000000,100000,10000,1000,100,10,1
v_number:        DD 0
v_div_bits:      DB 0
v_power:         DW 0
v_decimal:       DW 0
v_digits:        DB 0
v_digit:         DB 0
