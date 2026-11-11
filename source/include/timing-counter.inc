; Общие измерительные помощники приложений MXOS.
; Вызывающая программа задаёт timer_read и хранит v_elapsed DD, v_setup DD,
; v_last DW. elapsed/setup расположены подряд; постоянных данных DOS нет.
; timer_read возвращает убывающий HL и сохраняет BC/DE. Один интервал
; короче 65536 тиков; частота тика равна CLK1/256 и задаётся программой.
reset_timing:
    ; Обнулить elapsed/setup и защёлкнуть начало. AF/B/HL/флаги разрушены.
    xor a
    ld hl,v_elapsed
    ld b,8
reset_timing_loop:
    ld (hl),a
    inc hl
    dec b
    jp nz,reset_timing_loop
    call timer_read
    ld (v_last),hl
    ret

; Каждая ограниченная секция короче одного периода канала 2.
; Накапливаем разности по модулю 65536 в U32; прерывания не нужны.
; Все регистры и флаги сохраняются.
timer_sample:
    push af
    push bc
    push de
    push hl
    call timer_read
    push hl
    ex de,hl
    ld hl,(v_last)
    ld a,l
    sub e
    ld e,a
    ld a,h
    sbc a,d
    ld d,a
    pop hl
    ld (v_last),hl
    ld hl,v_elapsed
    call add_u16
    pop hl
    pop de
    pop bc
    pop af
    ret

; Вход DE: U16, HL: U32 LE. Разрушает AF/HL; BC/DE сохраняются.
add_u16:
    ld a,(hl)
    add a,e
    ld (hl),a
    inc hl
    ld a,(hl)
    adc a,d
    ld (hl),a
    inc hl
    ld a,(hl)
    adc a,0
    ld (hl),a
    inc hl
    ld a,(hl)
    adc a,0
    ld (hl),a
    ret
; Проверка движения каскада за заведомо заметную паузу вне измерений.
; Выход ZF=1 при неизменном счёте; разрушает все регистры.
timer_probe:
    call timer_read
    push hl
    ld de,1024
timer_probe_wait:
    dec de
    ld a,d
    or e
    jp nz,timer_probe_wait
    call timer_read
    pop de
    ld a,l
    sub e
    ld b,a
    ld a,h
    sbc a,d
    or b
    ret

