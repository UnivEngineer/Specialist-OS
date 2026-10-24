; TIME в секундах ВИ53, независимо от частоты и длительности прохода.
; Заставки, музыка загрузки и полная перерисовка не расходуют лимит.
; Все переменные принадлежат приложению, исторические карты не меняются.

; Синхронизировать базу без потери дробной секунды. Входа нет;
; все регистры и флаги сохраняются. Используется после блокирующей музыки.
level_timer_sync:
    PUSH AF
    PUSH HL
    CALL frame_clock_sample
    LD HL,(v_clock_ticks)
    LD (v_level_timer_last),HL
    POP HL
    POP AF
    RET

; После загрузки/полной перерисовки начать отсчёт с исходной дробной частью.
; Входа нет; все регистры и флаги сохраняются, рисуется начальный TIME.
level_timer_reset:
    PUSH AF
    PUSH BC
    PUSH DE
    PUSH HL
    CALL level_timer_sync
    LD HL,(v_level_timer_initial_remainder)
    LD (v_level_timer_remainder),HL
    CALL level_timer_print
    POP HL
    POP DE
    POP BC
    POP AF
    RET

; Входа нет. Выход и разрушаемые регистры — как у исходной печати TIME.
; timer_print_value — общий хвост timer_draw, заканчивающийся POP BC/RET.
; Сначала положить BC и перейти в хвост без добавления адреса возврата.
level_timer_print:
    PUSH BC
    JP timer_print_value

; Перед каждым проходом учесть прошедшие целые секунды и сохранить остаток.
; Входа нет; все регистры и флаги сохраняются до истечения лимита.
; При истечении используется исходный переход с переустановкой стека.
; Один отсчёт = 0,1 мс; перенос при сложении поддерживает сумму до 75534.
; Без целой секунды: 526 тактов, одна запись и два чтения ВИ53.
; Догоняние: до семи уменьшений цифр и один исходный вывод TIME.
level_timer_update:
    PUSH AF
    PUSH BC
    PUSH DE
    PUSH HL
    CALL frame_clock_sample
    LD HL,(v_level_timer_last)
    EX DE,HL
    LD HL,(v_clock_ticks)
    LD (v_level_timer_last),HL
    LD A,L
    SUB E
    LD L,A
    LD A,H
    SBC A,D
    LD H,A
    EX DE,HL
    LD HL,(v_level_timer_remainder)
    ADD HL,DE
    LD C,0
    JP NC,level_timer_split
    LD C,6
    LD DE,65536-6*PIT_TICK_HZ
    ADD HL,DE
level_timer_split:
    LD A,L
    SUB LOW(PIT_TICK_HZ)
    LD E,A
    LD A,H
    SBC A,HIGH(PIT_TICK_HZ)
    JP C,level_timer_remainder
    LD H,A
    LD L,E
    INC C
    JP level_timer_split
level_timer_remainder:
    LD (v_level_timer_remainder),HL
    LD A,C
    OR A
    JP Z,level_timer_done
level_timer_tick:
    CALL timer_borrow_units     ; Меняет цифры, fraction=4 подавляет старую печать.
    LD A,(v_time_hundreds)
    LD B,A
    LD A,(v_time_tens)
    OR B
    LD B,A
    LD A,(v_time_units)
    OR B
    JP Z,timer_expired
    DEC C
    JP NZ,level_timer_tick
    LD A,1
    LD (v_time_fraction),A
    CALL timer_draw            ; Один вывод/сигнал после догоняния целых секунд.
level_timer_done:
    POP HL
    POP DE
    POP BC
    POP AF
    RET

v_level_timer_last:      DW 0
v_level_timer_remainder: DW 0
v_level_timer_initial_remainder: DW 0

; Замена исходного LD (v_time_hundreds),A в конце level_load.
; Вход A=сотни старого лимита; все регистры и флаги сохраняются.
; Пересчёт выполняется один раз при загрузке, а не при перерисовке.
; Таблица вычисляется ассемблером из тех же пресетов, что и оригинал.
level_timer_convert:
    LD (v_time_hundreds),A
    PUSH AF
    PUSH BC
    PUSH DE
    PUSH HL
    LD A,(v_level)
    AND 00Fh
    ADD A
    ADD A
    LD E,A
    LD D,0
    LD HL,t_real_time_limits
    ADD HL,DE
    LD C,(HL)
    INC HL
    LD B,(HL)
    INC HL
    LD E,(HL)
    INC HL
    LD D,(HL)
    EX DE,HL
    LD (v_level_timer_initial_remainder),HL
    LD H,B
    LD L,C
    LD B,0
level_timer_convert_hundreds:
    LD A,L
    SUB 100
    LD E,A
    LD A,H
    SBC A,0
    JP C,level_timer_convert_tens_start
    LD H,A
    LD L,E
    INC B
    JP level_timer_convert_hundreds
level_timer_convert_tens_start:
    LD A,B
    LD (v_time_hundreds),A
    LD B,0
level_timer_convert_tens:
    LD A,L
    CP 10
    JP C,level_timer_convert_done
    SUB 10
    LD L,A
    INC B
    JP level_timer_convert_tens
level_timer_convert_done:
    LD A,B
    LD (v_time_tens),A
    LD A,L
    LD (v_time_units),A
    POP HL
    POP DE
    POP BC
    POP AF
    RET

; Запись: отображаемые секунды с округлением вверх и уже прошедшая
; доля первой секунды. Их разность точно сохраняет бюджет до 0,1 мс.
    MACRO REAL_TIME_LIMIT cycle_budget_value
        ASSERT (cycle_budget_value+ALMAZ_STEP_HZ-1)/ALMAZ_STEP_HZ <= 999
        DW (cycle_budget_value+ALMAZ_STEP_HZ-1)/ALMAZ_STEP_HZ
        IF cycle_budget_value % ALMAZ_STEP_HZ
            DW (ALMAZ_STEP_HZ-cycle_budget_value%ALMAZ_STEP_HZ)*PIT_TICK_HZ/ALMAZ_STEP_HZ
        ELSE
            DW 0
        ENDIF
    ENDM
t_real_time_limits:
    REAL_TIME_LIMIT t_time_budget_01
    REAL_TIME_LIMIT t_time_budget_02
    REAL_TIME_LIMIT t_time_budget_03
    REAL_TIME_LIMIT t_time_budget_04
    REAL_TIME_LIMIT t_time_budget_05
    REAL_TIME_LIMIT t_time_budget_06
    REAL_TIME_LIMIT t_time_budget_07
    REAL_TIME_LIMIT t_time_budget_08
    REAL_TIME_LIMIT t_time_budget_09
    REAL_TIME_LIMIT t_time_budget_10
    REAL_TIME_LIMIT t_time_budget_11
    REAL_TIME_LIMIT t_time_budget_12
    REAL_TIME_LIMIT t_time_budget_13
    REAL_TIME_LIMIT t_time_budget_14
    REAL_TIME_LIMIT t_time_budget_15
    REAL_TIME_LIMIT t_time_budget_16
