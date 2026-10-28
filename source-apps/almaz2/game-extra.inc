; Дополнительные переходники оптимизированной игры.

    IF ALMAZ_MONITOR_TARGET
; Выход в Монитор: сбросить активный ВИ53 при его наличии, установить
; белый цвет классического восьмицветного адаптера и очистить экран BIOS.
; Вход/выход: не возвращается; AF/BC/DE/HL и флаги могут изменяться.
monitor_game_exit:
    IF ALMAZ_PIT
        CALL audio_stop
    ENDIF
    IF ALMAZ_COLOR == 2
        LD A,082h
        LD (0FF03h),A
        XOR A
        LD (0FF02h),A
    ENDIF
    CALL bios_clearScreenOld
    JP app_bios_return
    ENDIF

    IF !ALMAZ_MONITOR_TARGET && ALMAZ_COLOR == 1
; Выход с цветным контроллером MX. Входа/выхода нет; уничтожаются AF/BC/HL,
; флаги. 40 байт кода и 2 байта ОЗУ, около 86900 тактов; 12 КБ экранных
; записей и одна запись порта цвета. Стек временно перемещается в экран и
; полностью восстанавливается перед RET. Обработка не использует прерывания.
soft_color_game_exit:
    CALL color_exit_clear
    JP app_bios_return
color_exit_clear:
    LD A,0F0h               ; Белый текст на чёрном фоне для перехода в ОС.
    LD (IO_COLOR),A
    LD HL,0
    ADD HL,SP
    LD (v_exit_stack),HL
    LD SP,0C000h
    LD HL,0
    LD BC,0300h
color_exit_clear_loop:
    PUSH HL
    PUSH HL
    PUSH HL
    PUSH HL
    PUSH HL
    PUSH HL
    PUSH HL
    PUSH HL
    DEC BC
    LD A,B
    OR C
    JP NZ,color_exit_clear_loop
    LD HL,(v_exit_stack)
    LD SP,HL
    RET
v_exit_stack: DW 0
    ENDIF

; Вход HL=таблица вступления. AF разрушается; BC/DE/HL сохраняются.
; Отложить обе части вступления до окончания вывода первой карты.
startup_mark:
    LD A,1
    LD (v_startup_pending),A
    RET

; Вход HL=музыка уровня; выход и регистры как у исходного проигрывателя.
; При первоначальной загрузке карта ещё не показана, поэтому музыки нет.
startup_level_music:
    LD A,(v_startup_pending)
    OR A
    RET NZ
    JP game_music_play

; Замена CALL map_draw в game_reload. AF/BC/DE/HL как у map_draw и музыки.
; После первого рисования проиграть вступление и мелодию загрузки уровня.
startup_map_draw:
    CALL map_draw
    LD A,(v_startup_pending)
    OR A
    RET Z
    XOR A
    LD (v_startup_pending),A
    LD HL,t_music_intro
    CALL game_music_play
    RET Z                   ; СТР отменяет обе части единой композиции.
    LD HL,t_music_level
    JP game_music_play

; Вход HL=мелодия; выход Z — отменена СТР, NZ — завершена. BC/DE
; сохраняются; AF/HL и флаги разрушаются. Отпускание СТР проверяется
; адресно даже при одновременном нажатии других клавиш. После ожидания
; синхронизируются часы, чтобы пауза не расходовала время уровня.
game_music_play:
    IF ALMAZ_PIT
        CALL audio_music
    ELSE
        CALL sound_sequence
    ENDIF
    CP 01Fh
    RET NZ
    LD A,082h
    LD (IO_KEYB_MODE),A
    LD A,00Fh
    LD (IO_KEYB_C),A
    LD A,0FEh
    LD (IO_KEYB_A),A
game_music_wait_release:
    LD A,(IO_KEYB_B)
    AND 080h
    JP Z,game_music_wait_release
    XOR A
    LD (IO_KEYB_A),A
    LD (IO_KEYB_C),A
    IF USE_FRAME_CLOCK
        CALL level_timer_sync
    ENDIF
    XOR A                   ; Z — отмена независимо от флагов BIOS/часов.
    RET

v_startup_pending: DB 0

    IF USE_FRAME_CLOCK
; Вместо random_sparse_event для грибов: сохраняет BC/DE/HL, возвращает
; A с ненулевым битом 0 для доли исходных успешных попыток. При 8 проходах/с
; доля равна 2/5: компенсация пересчёта времени в 2,5 реальных секунды.
; При другой частоте доля изменяется обратно пропорционально, максимум 1.
; Пять исходных вызовов ГПСЧ сохраняются; случайное направление выбирается
; только при разрешённой попытке, поэтому последовательность ГПСЧ меняется.
; Неуспешная исходная проверка: 35 дополнительных тактов с CALL;
; успешная: до 132 тактов, одно чтение и одна запись байта ОЗУ.
fungus_timed_event:
    CALL random_sparse_event
    AND 1
    RET Z
    LD A,(v_fungus_event_phase)
    ADD A,FUNGUS_GATE_INCREMENT
    JP C,fungus_event_allow
    CP FUNGUS_GATE_PERIOD
    JP C,fungus_event_deny
fungus_event_allow:
    SUB FUNGUS_GATE_PERIOD
    LD (v_fungus_event_phase),A
    LD A,1
    RET
fungus_event_deny:
    LD (v_fungus_event_phase),A
    XOR A
    RET

v_fungus_event_phase: DB 0
    ENDIF
