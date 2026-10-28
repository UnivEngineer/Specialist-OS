; Переходники ВИ53: игровые эффекты асинхронны, музыка заставки и загрузки
; использует исходный программный проигрыватель и BIOS-звук.

audio_game_start:
    LD SP,07FFFh
    CALL audio_init
    IF ALMAZ_OPTIMIZE
        CALL input_initialize
    ENDIF
    JP game_initialize
audio_game_exit:
    CALL audio_stop
    IF ALMAZ_OPTIMIZE && !ALMAZ_MONITOR_TARGET && ALMAZ_COLOR == 1
        CALL color_exit_clear
    ENDIF
    JP app_bios_return
audio_editor_new:
    LD SP,07FFFh
    CALL audio_init
    IF ALMAZ_OPTIMIZE
        CALL input_editor_release
        CALL input_initialize
    ENDIF
    JP editor_create_map
audio_editor_existing:
    LD SP,07FFFh
    CALL audio_init
    IF ALMAZ_OPTIMIZE
        CALL input_editor_release
        CALL input_initialize
    ENDIF
    JP editor_open

; Вход B=X/C=Y. Выход и разрушаемые регистры — как у map_get_cell.
; Без буфера и часов: 85 добавленных тактов при выключенном звуке,
; 185 при продолжающемся тоне, до 584 при смене тона.
; С обоими расширениями: 155 тактов без опроса стрелок и звука;
; верхняя граница со сменой звука и опросом указана в README.
audio_read_cell:
    CALL audio_update
    IF ALMAZ_OPTIMIZE
        CALL input_service
    ENDIF
    JP map_get_cell

; Музыка: HL=исходная байтовая таблица, BC/DE сохраняются,
; AF/HL меняются. Сначала отключить ВИ53, затем играть через BIOS.
; Заставка и загрузка остаются блокирующими; их время не расходует TIME.
audio_music:
    CALL audio_stop
    CALL music_sequence
    IF USE_FRAME_CLOCK
        CALL level_timer_sync
    ENDIF
    RET

; Редактор может ждать клавишу дольше периода счётчика: перед ожиданием
; обязательно убрать активную последовательность. A возвращается от BIOS.
audio_editor_getch:
    CALL audio_stop
    IF ALMAZ_OPTIMIZE
        JP input_editor_getch
    ELSE
        JP app_bios_getch
    ENDIF

; Последние секунды TIME: исходная игра задаёт оба параметра числом
; 30h+цифра. Десять записей рассчитаны ассемблером, умножения на 8080 нет.
; Все регистры и флаги сохраняются.
audio_countdown:
    PUSH AF
    PUSH BC
    PUSH DE
    PUSH HL
    LD A,(app_beep_frequency)
    SUB 030h
    CP 10
    JP NC,audio_countdown_done
    LD E,A
    LD D,0
    LD HL,0
    ADD HL,DE
    ADD HL,HL
    ADD HL,HL
    ADD HL,HL
    ADD HL,DE
    LD DE,t_countdown_sounds
    ADD HL,DE
    CALL audio_start
audio_countdown_done:
    POP HL
    POP DE
    POP BC
    POP AF
    RET

t_countdown_sounds:
    AUDIO_PRIORITY 2
    AUDIO_NOTE 030h,030h
    AUDIO_END
    AUDIO_PRIORITY 2
    AUDIO_NOTE 031h,031h
    AUDIO_END
    AUDIO_PRIORITY 2
    AUDIO_NOTE 032h,032h
    AUDIO_END
    AUDIO_PRIORITY 2
    AUDIO_NOTE 033h,033h
    AUDIO_END
    AUDIO_PRIORITY 2
    AUDIO_NOTE 034h,034h
    AUDIO_END
    AUDIO_PRIORITY 2
    AUDIO_NOTE 035h,035h
    AUDIO_END
    AUDIO_PRIORITY 2
    AUDIO_NOTE 036h,036h
    AUDIO_END
    AUDIO_PRIORITY 2
    AUDIO_NOTE 037h,037h
    AUDIO_END
    AUDIO_PRIORITY 2
    AUDIO_NOTE 038h,038h
    AUDIO_END
    AUDIO_PRIORITY 2
    AUDIO_NOTE 039h,039h
    AUDIO_END

