; Конфигурация игры. Флаги независимы от одноимённых настроек ОС.
    INCLUDE "../../source/include/mxos.inc"

    IFNDEF ALMAZ_COLOR
ALMAZ_COLOR = 1
    ENDIF
    IFNDEF ALMAZ_PIT
ALMAZ_PIT = 1
    ENDIF
    IFNDEF ALMAZ_RAMFOS
ALMAZ_RAMFOS = 0
    ENDIF
    IFNDEF ALMAZ_HUD
ALMAZ_HUD = 1
    ENDIF
    IFNDEF ALMAZ_OPTIMIZE
ALMAZ_OPTIMIZE = 1             ; Общий флаг оптимизаций игры и адресного ввода.
    ENDIF
    IFNDEF ALMAZ_MONITOR_TARGET
ALMAZ_MONITOR_TARGET = 0       ; Целевая оболочка — классический Монитор.
    ENDIF
    IFNDEF ALMAZ_STEP_HZ
ALMAZ_STEP_HZ = 8              ; Один проход карты; полный цикл содержит четыре.
    ENDIF
; Производная возможность оборудования, не отдельный переключатель сборки.
USE_FRAME_CLOCK = ALMAZ_OPTIMIZE && ALMAZ_PIT
    IFNDEF ANIMATION_QUOTA
ANIMATION_QUOTA = 10           ; Общий бюджет: Q=max(QUOTA-N_dynamic, MIN_QUOTA).
    ENDIF
    IFNDEF ANIMATION_MIN_QUOTA
ANIMATION_MIN_QUOTA = 3
    ENDIF
    IFNDEF ANIMATION_SCAN_LIMIT
ANIMATION_SCAN_LIMIT = 64       ; Ограничить и поиск кандидатов за один проход.
    ENDIF
    IFNDEF INPUT_POLL_CELL_STRIDE
INPUT_POLL_CELL_STRIDE = 43     ; 345/43*8 = около 64 опросов стрелок в секунду.
    ENDIF
    IFNDEF PLAYER_DIAGONAL_MOVEMENT
PLAYER_DIAGONAL_MOVEMENT = 0   ; Эксперимент: одновременное движение по двум осям.
    ENDIF
    IFNDEF PIT_CLOCK_HZ
PIT_CLOCK_HZ = 2000000         ; Вход ВИ53 для эффектов и часов, не для музыки.
    ENDIF
CPU_CLOCK_HZ = 2000000
PIT_TICK_HZ = 10000
FRAME_STEP_TICKS = PIT_TICK_HZ/ALMAZ_STEP_HZ
; 2/5 при 8 проходах/с: частота попыток относительно реального времени.
FUNGUS_GATE_PERIOD = 5*ALMAZ_STEP_HZ
FUNGUS_GATE_INCREMENT = 16
    IF FUNGUS_GATE_INCREMENT > FUNGUS_GATE_PERIOD
FUNGUS_GATE_INCREMENT = FUNGUS_GATE_PERIOD
    ENDIF
app_optimizations_enabled = ALMAZ_OPTIMIZE
app_monitor_target = ALMAZ_MONITOR_TARGET
app_input_enabled = ALMAZ_OPTIMIZE
app_frame_pacing_enabled = USE_FRAME_CLOCK
app_step_hz = ALMAZ_STEP_HZ
app_pit_clock_hz = PIT_CLOCK_HZ
app_realtime_enabled = USE_FRAME_CLOCK
app_animation_enabled = ALMAZ_OPTIMIZE
app_phases_enabled = ALMAZ_OPTIMIZE
app_input_cell_stride = INPUT_POLL_CELL_STRIDE
app_player_diagonal_movement = PLAYER_DIAGONAL_MOVEMENT
app_animation_quota = ANIMATION_QUOTA
app_animation_min_quota = ANIMATION_MIN_QUOTA
app_animation_scan = ANIMATION_SCAN_LIMIT
app_fast_enabled = ALMAZ_OPTIMIZE
MAP_BUFFER = 08000h

; Коды BIOS: F1..F8 = 00h..07h, F9..F11 = 8Ah..8Ch.
; На клавиатуре Специалиста F3 подписана NEW, F9 — EDIT.
; Адресный опрос автоматически вычисляет соответствующий столбец матрицы.
    IFNDEF KEY_EDITOR_NEW
KEY_EDITOR_NEW = 002h          ; F3 (NEW): создать новую карту вместо рабочей.
    ENDIF
    IFNDEF KEY_EDITOR_CURRENT
KEY_EDITOR_CURRENT = 08Ah      ; F9 (EDIT): редактировать текущую рабочую карту.
    ENDIF
    ASSERT (KEY_EDITOR_NEW >= 0 && KEY_EDITOR_NEW <= 7) || (KEY_EDITOR_NEW >= 08Ah && KEY_EDITOR_NEW <= 08Ch)
    ASSERT (KEY_EDITOR_CURRENT >= 0 && KEY_EDITOR_CURRENT <= 7) || (KEY_EDITOR_CURRENT >= 08Ah && KEY_EDITOR_CURRENT <= 08Ch)
    ASSERT KEY_EDITOR_NEW != KEY_EDITOR_CURRENT
app_key_editor_new = KEY_EDITOR_NEW
app_key_editor_current = KEY_EDITOR_CURRENT
EDITOR_NEW_COLUMN = 11-KEY_EDITOR_NEW
    IF KEY_EDITOR_NEW >= 08Ah
EDITOR_NEW_COLUMN = 3-(KEY_EDITOR_NEW-08Ah)
    ENDIF
EDITOR_CURRENT_COLUMN = 11-KEY_EDITOR_CURRENT
    IF KEY_EDITOR_CURRENT >= 08Ah
EDITOR_CURRENT_COLUMN = 3-(KEY_EDITOR_CURRENT-08Ah)
    ENDIF
EDITOR_NEW_PORT_A = 0FFh
EDITOR_NEW_PORT_C = 00Fh
    IF EDITOR_NEW_COLUMN < 8
EDITOR_NEW_PORT_A = 0FFh ^ (1 << EDITOR_NEW_COLUMN)
    ELSE
EDITOR_NEW_PORT_C = 00Fh ^ (1 << (EDITOR_NEW_COLUMN-8))
    ENDIF
EDITOR_CURRENT_PORT_A = 0FFh
EDITOR_CURRENT_PORT_C = 00Fh
    IF EDITOR_CURRENT_COLUMN < 8
EDITOR_CURRENT_PORT_A = 0FFh ^ (1 << EDITOR_CURRENT_COLUMN)
    ELSE
EDITOR_CURRENT_PORT_C = 00Fh ^ (1 << (EDITOR_CURRENT_COLUMN-8))
    ENDIF


; 0 — монохром, 1 — цвет MX, 2 — восьмицветный контроллер «М-К».
    ASSERT ALMAZ_COLOR >= 0 && ALMAZ_COLOR <= 2
app_color_port = IO_COLOR
app_hud_color = 070h
    IF ALMAZ_COLOR == 2
app_color_port = 0FF02h
app_hud_color = 000h
    ENDIF
    ASSERT ALMAZ_PIT == 0 || ALMAZ_PIT == 1
    ASSERT ALMAZ_RAMFOS == 0 || ALMAZ_RAMFOS == 1
    ASSERT ALMAZ_HUD == 0 || ALMAZ_HUD == 1
    ASSERT ALMAZ_OPTIMIZE == 0 || ALMAZ_OPTIMIZE == 1
    ASSERT ALMAZ_MONITOR_TARGET == 0 || ALMAZ_MONITOR_TARGET == 1
    ASSERT ANIMATION_QUOTA >= 1 && ANIMATION_QUOTA <= 64
    ASSERT ANIMATION_MIN_QUOTA >= 1 && ANIMATION_MIN_QUOTA <= ANIMATION_QUOTA
    ASSERT ANIMATION_SCAN_LIMIT >= 1 && ANIMATION_SCAN_LIMIT <= 255
    ASSERT INPUT_POLL_CELL_STRIDE >= 1 && INPUT_POLL_CELL_STRIDE <= 345
    ASSERT PLAYER_DIAGONAL_MOVEMENT == 0 || PLAYER_DIAGONAL_MOVEMENT == 1
    ASSERT !USE_FRAME_CLOCK || ALMAZ_PIT
    ASSERT ALMAZ_STEP_HZ >= 1 && ALMAZ_STEP_HZ <= 50
    ASSERT FRAME_STEP_TICKS < 08000h
    ASSERT PIT_CLOCK_HZ == 2000000 || PIT_CLOCK_HZ == 2500000

app_bios_getch        = bios_getch
app_bios_print_char   = bios_printChar
app_bios_print_string = bios_printString
    IF ALMAZ_RAMFOS
app_bios_return       = 0F800h
app_bios_beep         = 0C851h
app_bios_keycheck     = bios_keyScan
app_beep_frequency   = 0C64Eh
app_beep_duration    = 0C64Fh
app_cursor_position  = 0C63Eh
    ELSE
app_bios_return       = bios_reboot
app_bios_beep         = bios_beep_Old
app_bios_keycheck     = bios_keyCheck
app_beep_frequency   = bios_vars.beepFreq
app_beep_duration    = bios_vars.beepDuration
app_cursor_position  = bios_vars.cursorY
    ENDIF

; Один и тот же исходный список посылок для программного и аппаратного звука.
; Перевод периода и длительности выполняет ассемблер, не процессор 8080.
    MACRO AUDIO_PRIORITY priority
        IF ALMAZ_PIT
            DB priority
        ENDIF
    ENDM
    MACRO AUDIO_NOTE delay, repetitions
        ASSERT delay > 0 && delay < 0FFh
        ASSERT repetitions > 0 && repetitions <= 0FFh
        IF ALMAZ_PIT
            ASSERT (93+repetitions*(119+30*delay)+CPU_CLOCK_HZ/PIT_TICK_HZ-1)/(CPU_CLOCK_HZ/PIT_TICK_HZ) < 08000h
            DW ((119+30*delay)*(PIT_CLOCK_HZ/1000)+CPU_CLOCK_HZ/2000)/(CPU_CLOCK_HZ/1000)
            DW (93+repetitions*(119+30*delay)+CPU_CLOCK_HZ/PIT_TICK_HZ-1)/(CPU_CLOCK_HZ/PIT_TICK_HZ)
        ELSE
            DB delay, repetitions
        ENDIF
    ENDM
    MACRO AUDIO_REST
        IF ALMAZ_PIT
            DW 0, (98401+CPU_CLOCK_HZ/PIT_TICK_HZ-1)/(CPU_CLOCK_HZ/PIT_TICK_HZ)
        ELSE
            DB 0
        ENDIF
    ENDM
    MACRO AUDIO_END
        IF ALMAZ_PIT
            DW 0, 0
        ELSE
            DB 0FFh
        ENDIF
    ENDM

; Музыка всегда использует исходный программный формат и BIOS-звук.
    MACRO MUSIC_NOTE delay, repetitions
        DB delay, repetitions
    ENDM
    MACRO MUSIC_REST
        DB 0
    ENDM
    MACRO MUSIC_END
        DB 0FFh
    ENDM

; Исходные четыре байта пресета и его бюджет проходов до истечения.
; Пять полных циклов по четыре прохода на единицу; fraction — стартовая доля.
    MACRO LEVEL_TIME_PRESET preset_symbol, fraction, units, tens, hundreds
        DB fraction, units, tens, hundreds
preset_symbol = ((hundreds*100+tens*10+units)*5+fraction)*4
    ENDM
