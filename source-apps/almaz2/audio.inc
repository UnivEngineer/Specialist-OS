; Неблокирующий автомат ВИ53, размещённый вместо старых таблиц звука.
; Канал 0: режим 3, несущая. Канал 1: режим 2, 10 кГц.
; Канал 2: режим 0, длительность; OUT2 гасит звук аппаратно.
;
; audio_update: входа нет; все регистры и флаги сохраняются.
; Нулевой счёт либо отрицательная половина после переполнения означают
; конец посылки. Каждая длительность < 32768 отсчётов; автомат обслуживается
; при обходе клеток и перерисовке. Перед музыкой и блокирующим
; вводом вызывается audio_stop, поэтому долгий простой не маскирует конец.
audio_update:
    PUSH AF
    LD A,(v_audio_active)
    OR A
    JP Z,audio_update_return
    PUSH HL
    LD A,080h                 ; Защёлкнуть счётчик 2: команда именно 8253.
    LD (IO_TIMER_MODE),A
    LD A,(IO_TIMER_CH2)
    LD L,A
    LD A,(IO_TIMER_CH2)
    OR A
    JP M,audio_update_next
    OR L
    JP NZ,audio_update_restore
audio_update_next:
    PUSH BC
    PUSH DE
    CALL audio_next
    POP DE
    POP BC
audio_update_restore:
    POP HL
audio_update_return:
    POP AF
    RET

; Следующая запись: DW делитель, длительность. 0/неноль — пауза,
; 0/0 — конец. Вход: v_audio_pointer; AF/BC/DE/HL разрушаются.
; Последовательности не сжимаются: короткие неравные посылки сохраняются,
; а моменты обслуживания вносят небольшую зависимость тембра от сцены.
audio_next:
    LD HL,(v_audio_pointer)
    LD E,(HL)
    INC HL
    LD D,(HL)
    INC HL
    LD C,(HL)
    INC HL
    LD B,(HL)
    INC HL
    LD (v_audio_pointer),HL
    LD A,B
    OR C
    JP Z,audio_stop
    LD A,036h
    LD (IO_TIMER_MODE),A
    LD A,D
    OR E
    JP Z,audio_next_duration
    LD A,E
    LD (IO_TIMER_CH0),A
    LD A,D
    LD (IO_TIMER_CH0),A
audio_next_duration:
    IF USE_FRAME_CLOCK
        CALL frame_clock_sample_saved
        LD H,B
        LD L,C
        INC HL
        INC HL                ; Около 0,2 мс от защёлки до загрузки нового счёта.
        LD (v_clock_previous),HL
    ENDIF
    LD A,0B0h
    LD (IO_TIMER_MODE),A
    LD A,C
    LD (IO_TIMER_CH2),A
    LD A,B
    LD (IO_TIMER_CH2),A
    RET

; Остановить звук без ожидания; AF разрушается, остальные сохраняются.
; Без часов режим 1 держит OUT2=1. С часами канал 2 считает свободно,
; а режим 3 без счёта держит OUT0=1: переменного сигнала нет в обоих случаях.
audio_stop:
    IF USE_FRAME_CLOCK
        CALL frame_clock_idle
    ELSE
    LD A,0B2h
    LD (IO_TIMER_MODE),A
    ENDIF
    LD A,036h
    LD (IO_TIMER_MODE),A
    XOR A
    LD (v_audio_active),A
    LD (v_audio_priority),A
    RET

v_audio_pointer:  DW 0
v_audio_priority: DB 0
v_audio_active:   DB 0

; Инициализация один раз на старте; AF разрушается.
audio_init:
    IF USE_FRAME_CLOCK
        PUSH HL
    ENDIF
    CALL audio_stop
    LD A,074h
    LD (IO_TIMER_MODE),A
    LD A,LOW(PIT_CLOCK_HZ/PIT_TICK_HZ)
    LD (IO_TIMER_CH1),A
    LD A,HIGH(PIT_CLOCK_HZ/PIT_TICK_HZ)
    LD (IO_TIMER_CH1),A
    LD A,00Ah                 ; Программный звук PC5 должен быть выключен.
    LD (IO_KEYB_MODE),A
    IF USE_FRAME_CLOCK
        XOR A
        LD (v_frame_ready),A
        LD H,A
        LD L,A
        LD (v_clock_ticks),HL
        LD (v_clock_previous),HL
        POP HL
    ENDIF
    RET

; Принять эффект HL. Первый байт — приоритет 0..3. Активный эффект
; вытесняется только более важным; равные запросы не создают очередь.
; Все регистры и флаги сохраняются, ожиданий и сканирования клавиатуры нет.
audio_start:
    PUSH AF
    PUSH BC
    PUSH DE
    PUSH HL
    LD B,(HL)
    LD A,(v_audio_active)
    OR A
    JP Z,audio_start_accept
    LD A,(v_audio_priority)
    CP B
    JP NC,audio_start_done
audio_start_accept:
    LD A,B
    LD (v_audio_priority),A
    INC HL
    LD (v_audio_pointer),HL
    LD A,1
    LD (v_audio_active),A
    CALL audio_next
audio_start_done:
    POP HL
    POP DE
    POP BC
    POP AF
    RET
