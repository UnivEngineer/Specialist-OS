; Быстрый ввод стрелок без BIOS, раскладок, автоповтора и задержки дребезга.
; Стандартная матрица: строка B2; вправо A2, влево A4, вниз C0, вверх C1.
; Команды интерфейса читаются адресно; блокирующий ввод редактора — через BIOS.

; Входа нет. Все регистры и флаги сохраняются.
input_initialize:
    PUSH AF
    LD A,0FFh
    LD (v_input_pending),A
    LD (v_input_held_code),A
    XOR A
    LD (v_input_mask),A
    LD (v_input_selected),A
    POP AF
    RET

; Вход B=X. Опрос на столбцах, кратных INPUT_POLL_STRIDE: по умолчанию
; не более 4 клеток внутри строки и 7 на стыке. Регистры и флаги сохраняются.
; 53 такта без опроса; до 852 с опросом; 13 обращений к ВВ55 при опросе.
input_service:
    PUSH AF
    LD A,B
    AND INPUT_POLL_STRIDE-1
    JP NZ,input_service_done
    CALL input_poll
input_service_done:
    POP AF
    RET

; Входа нет. Все регистры и флаги сохраняются. 495..782 такта, 4 чтения B,
; 9 записей ВВ55, 194 байта кода до input_take_direction.
; По отпусканию pending не стирается: короткое нажатие живёт до шага игрока.
; При нескольких удерживаемых стрелках новая имеет приоритет над старой;
; порядок одновременных фронтов: влево, вправо, вверх, вниз.
input_poll:
    PUSH AF
    PUSH BC
    PUSH DE
    PUSH HL
    LD A,082h
    LD (IO_KEYB_MODE),A
    LD A,00Fh
    LD (IO_KEYB_C),A
    LD A,0FBh
    LD (IO_KEYB_A),A
    LD A,(IO_KEYB_B)
    AND 4
    LD D,0
    JP NZ,input_poll_left
    LD D,2
input_poll_left:
    LD A,0EFh
    LD (IO_KEYB_A),A
    LD A,(IO_KEYB_B)
    AND 4
    JP NZ,input_poll_down
    LD A,D
    OR 1
    LD D,A
input_poll_down:
    LD A,0FFh
    LD (IO_KEYB_A),A
    LD A,00Eh
    LD (IO_KEYB_C),A
    LD A,(IO_KEYB_B)
    AND 4
    JP NZ,input_poll_up
    LD A,D
    OR 8
    LD D,A
input_poll_up:
    LD A,00Dh
    LD (IO_KEYB_C),A
    LD A,(IO_KEYB_B)
    AND 4
    JP NZ,input_poll_restore
    LD A,D
    OR 4
    LD D,A
input_poll_restore:
    XOR A
    LD (IO_KEYB_A),A
    LD (IO_KEYB_C),A
    LD A,(v_input_mask)
    CPL
    AND D
    LD E,A
    LD A,D
    LD (v_input_mask),A
    LD C,1
    LD A,E
    OR A
    JP NZ,input_choose
    LD C,0
    LD A,(v_input_selected)
    AND D
    JP NZ,input_poll_done
    LD A,D
    OR A
    JP Z,input_released
    LD E,A
input_choose:
    LD A,E
    AND 1
    LD B,1
    LD A,008h
    JP NZ,input_selected
    LD A,E
    AND 2
    LD B,2
    LD A,018h
    JP NZ,input_selected
    LD A,E
    AND 4
    LD B,4
    LD A,019h
    JP NZ,input_selected
    LD B,8
    LD A,01Ah
input_selected:
    LD (v_input_held_code),A
    LD E,A
    LD A,B
    LD (v_input_selected),A
    LD A,C
    OR A
    JP Z,input_poll_done
    LD A,E
    LD (v_input_pending),A
    JP input_poll_done
input_released:
    XOR A
    LD (v_input_selected),A
    LD A,0FFh
    LD (v_input_held_code),A
input_poll_done:
    POP HL
    POP DE
    POP BC
    POP AF
    RET

; Выход A: последнее новое нажатие, иначе удерживаемая стрелка, иначе FFh.
; Накопленное нажатие расходуется ровно один раз. BC/DE/HL сохраняются;
; флаги меняются. Удержание даёт движение на каждом шаге без BIOS-повтора.
input_take_direction:
    CALL input_poll
    LD A,(v_input_pending)
    CP 0FFh
    JP NZ,input_take_pending
    LD A,(v_input_held_code)
    RET
input_take_pending:
    PUSH AF
    LD A,0FFh
    LD (v_input_pending),A
    POP AF
    RET

; После полной перерисовки не переносить старый ввод на новую карту.
input_map_drawn:
    CALL hud_draw_level
    IF ALMAZ_OPTIMIZE
        CALL animation_initialize
    ENDIF
    JP input_initialize

    IF !ALMAZ_PIT
input_read_cell:
    CALL input_service
    JP map_get_cell
input_game_start:
    LD SP,07FFFh
    CALL input_initialize
    JP game_initialize
input_editor_new:
    LD SP,07FFFh
    CALL input_editor_release
    CALL input_initialize
    JP editor_create_map
input_editor_existing:
    LD SP,07FFFh
    CALL input_editor_release
    CALL input_initialize
    JP editor_open
    ENDIF

; Выход A=команда либо FFh; BC/DE/HL сохраняются, флаги меняются.
; Вход в редактор разрешён только из игры/редактора, не из музыки или звука.
; Переходники по 0003h/0006h сбрасывают стек и останавливают звук.
input_game_command:
    CALL input_read_command
    JP input_editor_dispatch

; Блокирующий BIOS-ввод редактора с перехватом функциональных клавиш:
; они не должны попадать в editor_select_tile как номер тайла.
; Выход и разрушаемые регистры как у app_bios_getch, флаги меняются.
input_editor_getch:
    CALL app_bios_getch
input_editor_dispatch:
    CP KEY_EDITOR_NEW
    JP Z,game_new_editor_map
    CP KEY_EDITOR_CURRENT
    JP Z,game_edit_current_map
    RET

; Дождаться отпускания команд перед открытием редактора. BC/DE/HL
; сохраняются, AF меняется. Не даёт удерживаемой NEW пересоздать карту.
input_editor_release:
    CALL input_read_command
    CP 0FFh
    JP NZ,input_editor_release
    RET

; Входа нет. A=20h/3Bh/2Dh/7Ah/1Fh, код редактора либо FFh;
; BC/DE/HL сохраняются, флаги меняются. Стрелки не вытесняют команды.
; Пять старых команд сохраняют относительный приоритет BIOS. Редактор
; имеет приоритет над ними; новая карта — над редактированием текущей.
; 145 байт, вместе с CALL 75..598 тактов (до 0,299 мс при 2 МГц),
; 2 обращения к ВВ55 без клавиш, до 20 с клавишами. Режим 82h сбрасывает
; выходы портов; после подробного опроса A/C также возвращаются в ноль.
input_read_command:
    LD A,082h
    LD (IO_KEYB_MODE),A
    LD A,(IO_KEYB_B)
    OR 3
    CP 0FFh
    RET Z
    PUSH BC
    LD B,0FFh
    LD A,00Fh
    LD (IO_KEYB_C),A
    LD A,0FEh
    LD (IO_KEYB_A),A
    LD A,(IO_KEYB_B)
    LD C,A
    AND 040h
    JP NZ,input_command_exit
    LD B,02Dh
input_command_exit:
    LD A,C
    AND 080h
    JP NZ,input_command_store
    LD B,01Fh
input_command_store:
    LD A,0FBh
    LD (IO_KEYB_A),A
    LD A,(IO_KEYB_B)
    AND 020h
    JP NZ,input_command_space
    LD B,07Ah
input_command_space:
    LD A,0DFh
    LD (IO_KEYB_A),A
    LD A,(IO_KEYB_B)
    AND 4
    JP NZ,input_command_next
    LD B,020h
input_command_next:
    LD A,0FFh
    LD (IO_KEYB_A),A
    LD A,7
    LD (IO_KEYB_C),A
    LD A,(IO_KEYB_B)
    AND 040h
    JP NZ,input_command_editor_current
    LD B,03Bh
input_command_editor_current:
    LD A,EDITOR_CURRENT_PORT_A
    LD (IO_KEYB_A),A
    LD A,EDITOR_CURRENT_PORT_C
    LD (IO_KEYB_C),A
    LD A,(IO_KEYB_B)
    AND 080h
    JP NZ,input_command_editor_new
    LD B,KEY_EDITOR_CURRENT
input_command_editor_new:
    LD A,EDITOR_NEW_PORT_A
    LD (IO_KEYB_A),A
    LD A,EDITOR_NEW_PORT_C
    LD (IO_KEYB_C),A
    LD A,(IO_KEYB_B)
    AND 080h
    JP NZ,input_command_done
    LD B,KEY_EDITOR_NEW
input_command_done:
    XOR A
    LD (IO_KEYB_A),A
    LD (IO_KEYB_C),A
    LD A,B
    POP BC
    RET

v_input_pending:    DB 0FFh
v_input_held_code:  DB 0FFh
v_input_mask:       DB 0
v_input_selected:   DB 0
