; Визуальное расписание отделено от физики и её ГПСЧ.
; Курсор с нечётным шагом обходит все 512 адресов карты без повторов.
; На проход отводится max(QUOTA-N_dynamic, MIN_QUOTA) рисунков и до SCAN
; проверок адресов. Перепись динамических объектов ограничена QUOTA-MIN_QUOTA.
; Когда все статичные объекты помещаются в квоту, они рисуются в логике
; каждого прохода. При перегрузке действует ограниченный случайный обход:
; его фаза меняется по завершении полного обхода адресов.

; Вход HL=банк прохода; все регистры и флаги сохраняются.
animation_begin_bank:
    IF USE_FRAME_CLOCK
        CALL frame_begin_bank
    ELSE
        LD (v_sprite_bank),HL
    ENDIF
    PUSH AF
    PUSH BC
    PUSH DE
    PUSH HL
    IF ALMAZ_OPTIMIZE
        CALL animation_step
    ENDIF
    POP HL
    POP DE
    POP BC
    POP AF
    RET

; Входа нет; все регистры и флаги сохраняются. Визуальный ГПСЧ имеет
; собственное состояние, не читает и не меняет v_random_state игры.
animation_initialize:
    PUSH AF
    PUSH BC
    PUSH DE
    PUSH HL
    XOR A
    LD (v_animation_active),A
    LD (v_animation_sweep),A
    LD (v_animation_direct),A
    LD A,ANIMATION_QUOTA+1
    LD (v_animation_static_count),A
    LD A,(v_animation_seed)
    LD B,A
    LD A,(v_level)
    XOR B
    OR 1
    LD (v_animation_seed),A
    CALL animation_random
    LD L,A
    CALL animation_random
    AND 1
    LD H,A
    LD DE,MAP_BUFFER
    ADD HL,DE
    LD (v_animation_cursor),HL
    CALL animation_random
    OR 1
    LD L,A
    LD H,0
    LD (v_animation_stride),HL
    LD HL,0200h
    LD (v_animation_walk_left),HL
    CALL animation_random
    LD (v_animation_salt),A
    POP HL
    POP DE
    POP BC
    POP AF
    RET

; Выход A=следующий визуальный байт; остальные регистры сохраняются.
; Ненулевое состояние сдвигается вправо с обратной связью B8h.
animation_random:
    LD A,(v_animation_seed)
    OR A
    RRA
    JP NC,animation_random_store
    XOR 0B8h
animation_random_store:
    LD (v_animation_seed),A
    RET

; Только два исходных места чистой анимации: стоящий алмаз и гриб.
; Вход A=тип, B/C=координаты. AF/BC/HL сохраняются; DE может меняться
; при переходе к обычному рисованию, как у tile_draw.
; При квоте: 48 байт, отложенный вызов 115..133 такта; прямое рисование
; до 1579 тактов, 32 записи экрана и одна запись цвета в цветной сборке.
animation_static_draw:
    PUSH AF
    IF ALMAZ_OPTIMIZE
    LD A,(v_animation_static_count)
    CP ANIMATION_QUOTA+1
    JP NC,animation_static_counted
    INC A
    LD (v_animation_static_count),A
animation_static_counted:
    ENDIF
    LD A,(v_animation_active)
    OR A
    JP Z,animation_static_immediate
    IF ALMAZ_OPTIMIZE
    LD A,(v_animation_direct)
    OR A
    JP Z,animation_static_deferred
    LD A,(v_animation_quota_left)
    OR A
    JP Z,animation_static_deferred
    DEC A
    LD (v_animation_quota_left),A
    POP AF
    JP tile_draw
    ENDIF
animation_static_deferred:
    POP AF
    RET
animation_static_immediate:
    POP AF
    JP tile_draw

    IF ALMAZ_OPTIMIZE
; Вызывается с сохранёнными регистрами. AF/BC/DE/HL разрушаются.
; Падение, появление, рост и удаление рисуются исходным кодом немедленно.
; Перепись читает максимум 345 клеток, прекращаясь при насыщении квоты.
; Поиск кандидатов и рисование дополнительно ограничены SCAN и QUOTA.
; Выбор прямого режима использует число статичных вызовов прошлого прохода.
; Счёт насыщается на QUOTA+1: точное большее число не требуется. Прямое
; рисование всё равно проверяет остаток Q, поэтому рост числа объектов
; внутри прохода не может превысить квоту. Физика и рост не откладываются.
; При SCAN=64/QUOTA=10/MIN=3 верхняя оценка поиска и рисований 69391 такт
; (34,7 мс на 2 МГц), до 320 записей экрана и 185 обращений к оборудованию.
animation_step:
    LD A,1
    LD (v_animation_active),A
    CALL animation_choose_quota
    LD (v_animation_quota),A
    LD (v_animation_quota_left),A
    XOR A
    LD (v_animation_direct),A
    LD A,(v_animation_static_count)
    OR A
    RET Z
    LD B,A
    XOR A
    LD (v_animation_static_count),A
    LD A,(v_animation_quota)
    CP B
    JP NC,animation_direct_step
    LD A,ANIMATION_SCAN_LIMIT
    LD (v_animation_scan_left),A
    LD HL,(v_animation_walk_left)
    LD B,H
    LD C,L
    LD HL,(v_animation_stride)
    EX DE,HL
    LD HL,(v_animation_cursor)
animation_scan:
    ADD HL,DE
    LD A,H
    AND 1
    OR HIGH(MAP_BUFFER)
    LD H,A
    DEC BC
    LD A,B
    OR C
    JP NZ,animation_same_sweep
    LD BC,0200h
    LD A,(v_animation_sweep)
    INC A
    AND 3
    LD (v_animation_sweep),A
animation_same_sweep:
    LD A,L
    AND 01Fh
    CP 018h
    JP NC,animation_scan_next
    LD A,(HL)
    CP 5
    JP Z,animation_selected
    CP 8
    JP NZ,animation_scan_next
animation_selected:
    PUSH BC
    PUSH DE
    PUSH HL
    PUSH AF
    LD A,L
    AND 01Fh
    LD B,A
    LD A,L
    RRCA
    RRCA
    RRCA
    RRCA
    RRCA
    AND 7
    LD C,A
    LD A,H
    AND 1
    ADD A
    ADD A
    ADD A
    OR C
    LD C,A
    LD HL,(v_sprite_bank)
    EX (SP),HL              ; На стеке старый банк, HL содержит сохранённый AF.
    PUSH HL                ; Сохранить AF до выбора банка полного обхода.
    LD A,(v_animation_sweep)
    ADD A
    LD HL,t_animation_banks
    ADD A,L
    LD L,A
    LD E,(HL)
    INC HL
    LD D,(HL)
    EX DE,HL
    LD (v_sprite_bank),HL
    POP AF
    CALL tile_draw
    POP HL
    LD (v_sprite_bank),HL
    POP HL
    POP DE
    POP BC
    LD A,(v_animation_quota_left)
    DEC A
    LD (v_animation_quota_left),A
    JP Z,animation_scan_done
animation_scan_next:
    LD A,(v_animation_scan_left)
    DEC A
    LD (v_animation_scan_left),A
    JP Z,animation_scan_done
    LD A,C
    AND 00Fh
    JP NZ,animation_scan
animation_service:
    IF ALMAZ_PIT
        CALL audio_update
    ENDIF
    JP animation_scan
animation_scan_done:
    LD (v_animation_cursor),HL
    LD H,B
    LD L,C
    LD (v_animation_walk_left),HL
    RET

; Все кандидаты помещаются в Q: пропустить поиск по пустым клеткам.
; tile_draw в исходных точках логики использует банк текущего прохода,
; поэтому каждый оставшийся алмаз/гриб меняет фазу каждый проход.
animation_direct_step:
    LD A,1
    LD (v_animation_direct),A
    RET

; Выход A=Q, C=Q; AF/BC/DE/HL разрушаются. Клетки X=1..23, Y=1..15,
; как у логики игры; рамка сверху/слева и заполнение строк не учитываются.
; Игрок/выход исключены. Динамика: 6/7, дым 00h с ненулевыми флагами,
; камни/алмазы 4/5 с состоянием движения в старшей тетраде.
; Достаточно найти QUOTA-MIN_QUOTA объектов, а не полное их количество.
; Таблица вычитает единицу без ветвления по типу. Через каждые четыре строки
; обслуживаются звук/стрелки; карта не меняется, игровой ГПСЧ не вызывается.
; 54 байта кода + 256 байт таблицы с выравниванием; при 20/10 от 693 тактов
; (первые десять клеток динамические) до 29658 (полная перепись, 14,9 мс).
; До 345 чтений карты, 345 чтений таблицы и 75 обращений к оборудованию.
animation_choose_quota:
    LD C,ANIMATION_QUOTA
    IF ANIMATION_QUOTA != ANIMATION_MIN_QUOTA
    LD HL,MAP_BUFFER+021h
    LD D,HIGH(t_animation_dynamic)
    LD B,15
animation_count_cell:
    LD E,(HL)
    LD A,(DE)
    ADD A,C
    LD C,A
    CP ANIMATION_MIN_QUOTA
    RET Z
    INC HL
    LD A,L
    AND 01Fh
    CP 018h
    JP NZ,animation_count_cell
    LD A,L
    ADD 9
    LD L,A
    JP NC,animation_count_row
    INC H
animation_count_row:
    DEC B
    JP Z,animation_count_done
    LD A,B
    AND 3
    JP NZ,animation_count_cell
    IF ALMAZ_PIT
        CALL audio_update
    ENDIF
    JP animation_count_cell
    ENDIF
animation_count_done:
    LD A,C
    RET
    ENDIF

    IF ALMAZ_OPTIMIZE
; Замена LD HL,(v_sprite_bank). Вход A=тип, B/C=координаты.
; Выход HL=банк с индивидуальным сдвигом; AF/BC/DE и флаги сохраняются.
; Координатный сдвиг влияет только на рисунок, не на состояние объекта.
; 68 байт, 132..292 такта, обращений к оборудованию нет.
animation_sprite_bank:
    LD HL,(v_sprite_bank)
    PUSH AF
    CP 5
    JP Z,animation_shift_bank
    CP 6
    JP Z,animation_shift_bank
    CP 7
    JP Z,animation_shift_bank
    CP 8
    JP Z,animation_shift_bank
    CP 00Bh
    JP Z,animation_shift_bank
    POP AF
    RET
animation_shift_bank:
    PUSH DE
    LD A,H
    SUB 00Ch
    CP 2
    JP C,animation_bank_index
    DEC A
animation_bank_index:
    LD D,A
    LD A,B
    RRCA
    RRCA
    XOR B
    XOR C
    LD E,A
    LD A,(v_animation_salt)
    XOR E
    ADD A,D
    AND 3
    ADD A
    LD HL,t_animation_banks
    ADD A,L
    LD L,A
    LD E,(HL)
    INC HL
    LD D,(HL)
    EX DE,HL
    POP DE
    POP AF
    RET
    ENDIF

animation_map_drawn:
    CALL hud_draw_level
    JP animation_initialize

    ALIGN 8, 0FFh
t_animation_banks:
    DW t_sprites_phase_zero, t_sprites_phase_one
    DW t_sprites_phase_two, t_sprites_phase_three

v_animation_seed:       DB 0A5h
v_animation_salt:       DB 0
v_animation_active:     DB 0
v_animation_direct:     DB 0
v_animation_static_count: DB ANIMATION_QUOTA+1
v_animation_sweep:      DB 0
v_animation_cursor:    DW MAP_BUFFER
v_animation_stride:    DW 1
v_animation_walk_left: DW 0200h
v_animation_quota:      DB 0
v_animation_quota_left: DB 0
v_animation_scan_left: DB 0

    IF ALMAZ_OPTIMIZE && ANIMATION_QUOTA != ANIMATION_MIN_QUOTA
; Индексация сырым байтом карты: 0FFh уменьшает остаток бюджета на единицу.
; Выравнивание позволяет использовать D как неизменную старшую часть адреса.
    ALIGN 256, 0FFh
t_animation_dynamic:
cell_value = 0
    DUP 256
        IF (cell_value & 15) == 6 || (cell_value & 15) == 7 || ((cell_value & 0F0h) != 0 && ((cell_value & 15) == 0 || (cell_value & 15) == 4 || (cell_value & 15) == 5))
            DB 0FFh
        ELSE
            DB 0
        ENDIF
cell_value = cell_value+1
    EDUP
    ENDIF
