; Единые часы 10 кГц на счётчике 2, совместно с отсечкой звука.
; Перед каждой перезагрузкой счётчика накапливается истекшее время,
; затем сохраняется новое начальное значение. Свободный счётчик продолжает
; считать с 0000h (65536) в режиме 0; канал 0 при этом выключен.
; Переполнение раз в 6,5536 с допустимо, если между выборками меньше периода.
; В редакторе часы не используются для темпа; полная перерисовка сбрасывает
; привязку шага, поэтому длительный ввод и музыка не требуют догонять игру.

; Выход HL=защёлкнутый счётчик 2. AF/флаги меняются, BC/DE сохраняются.
; 38 байт, 214 тактов. Одна запись и два чтения ВИ53; накапливается
; 16-битное время в отсчётах. Переходники перезагрузки компенсируют
; интервал от защёлки до нового старта двумя отсчётами (около 0,2 мс).
frame_clock_sample:
    PUSH DE
    LD A,080h
    LD (IO_TIMER_MODE),A
    LD A,(IO_TIMER_CH2)
    LD E,A
    LD A,(IO_TIMER_CH2)
    LD D,A
    LD HL,(v_clock_previous)
    LD A,L
    SUB E
    LD L,A
    LD A,H
    SBC A,D
    LD H,A
    EX DE,HL
    LD (v_clock_previous),HL
    PUSH HL
    LD HL,(v_clock_ticks)
    ADD HL,DE
    LD (v_clock_ticks),HL
    POP HL
    POP DE
    RET

; Вариант для проигрывателя: HL также сохраняется; AF/флаги меняются.
frame_clock_sample_saved:
    PUSH HL
    CALL frame_clock_sample
    POP HL
    RET

; Остановить отсечку звука и оставить непрерывный отсчёт времени.
; AF/флаги меняются; BC/DE/HL сохраняются. Канал 0 выключает audio_stop.
frame_clock_idle:
    PUSH HL
    CALL frame_clock_sample
    LD A,0B0h
    LD (IO_TIMER_MODE),A
    XOR A
    LD (IO_TIMER_CH2),A
    LD (IO_TIMER_CH2),A
    LD HL,2
    LD (v_clock_previous),HL
    POP HL
    RET

; Выход HL=время от начала предыдущего прохода, DE меняется.
frame_elapsed:
    LD HL,(v_frame_start)
    EX DE,HL
    LD HL,(v_clock_ticks)
    LD A,L
    SUB E
    LD L,A
    LD A,H
    SBC A,D
    LD H,A
    RET

; Вход HL=банк спрайтов; заменить исходную запись v_sprite_bank.
; Все регистры и флаги сохраняются. Один шаг — один из четырёх проходов
; карты. Ожидание обслуживает звук и ввод, тяжёлый проход не догоняется.
; Период считается от фактического начала шага, а не от конца вычислений.
frame_begin_bank:
    LD (v_sprite_bank),HL
    PUSH AF
    PUSH BC
    PUSH DE
    PUSH HL
    CALL frame_clock_sample
    LD A,(v_frame_ready)
    OR A
    JP Z,frame_begin_start
    CALL frame_elapsed
    LD (v_step_work_ticks),HL
    CALL frame_record_work
    LD HL,(v_step_work_ticks)
frame_begin_check:
    LD A,L
    SUB LOW(FRAME_STEP_TICKS)
    LD A,H
    SBC A,HIGH(FRAME_STEP_TICKS)
    JP NC,frame_begin_ready
    CALL audio_update
    CALL frame_clock_sample
    CALL frame_elapsed
    JP frame_begin_check
frame_begin_ready:
    LD (v_step_period_ticks),HL
frame_begin_start:
    LD HL,(v_clock_ticks)
    LD (v_frame_start),HL
    LD A,1
    LD (v_frame_ready),A
    IF USE_FRAME_CLOCK
        CALL level_timer_update
    ENDIF
    POP HL
    POP DE
    POP BC
    POP AF
    RET

; Вход HL=работа предыдущего прохода в отсчётах 0,1 мс.
; AF/DE/HL разрушаются; BC сохраняется. Без дополнительных обращений ВИ53.
; Превышение строго больше бюджета; счётчик насыщается на 65535.
; 35 байт, 81..167 тактов; обращений к оборудованию нет.
frame_record_work:
    EX DE,HL
    LD HL,(v_step_peak_ticks)
    LD A,L
    SUB E
    LD A,H
    SBC A,D
    JP NC,frame_record_deadline
    LD H,D
    LD L,E
    LD (v_step_peak_ticks),HL
frame_record_deadline:
    LD A,LOW(FRAME_STEP_TICKS)
    SUB E
    LD A,HIGH(FRAME_STEP_TICKS)
    SBC A,D
    RET NC
    LD HL,(v_step_overruns)
    LD A,H
    AND L
    INC A
    RET Z
    INC HL
    LD (v_step_overruns),HL
    RET

; Вызывается вместо hud_draw_level в конце полной перерисовки карты.
; Отменить старый срок: первая логика после загрузки начинается сразу.
frame_map_drawn:
    CALL hud_draw_level
    IF ALMAZ_OPTIMIZE
        CALL animation_initialize
    ENDIF
    IF ALMAZ_OPTIMIZE
        CALL input_initialize
    ENDIF
    PUSH AF
    PUSH HL
    XOR A
    LD (v_frame_ready),A
    LD HL,0
    LD (v_step_work_ticks),HL
    LD (v_step_period_ticks),HL
    LD (v_step_peak_ticks),HL
    LD (v_step_overruns),HL
    POP HL
    POP AF
    IF USE_FRAME_CLOCK
        CALL level_timer_reset
    ENDIF
    RET

v_clock_previous:   DW 0
v_clock_ticks:      DW 0
v_frame_start:      DW 0
v_frame_ready:      DB 0
v_step_work_ticks:  DW 0
v_step_period_ticks: DW 0
v_step_peak_ticks:   DW 0
v_step_overruns:     DW 0
