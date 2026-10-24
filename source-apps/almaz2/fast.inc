; Быстрый вход только из четырёх исходных проходов карты: B=1..23, C=1..15.
; Порядок клеток, обслуживание ВИ53 и частота опроса стрелок не меняются.
; Оригинальные map_get_cell/map_put_cell и обработчики движений сохранены.

; Вход B=X, C=Y. AF/DE/HL разрушаются; BC — по контракту обработчика
; объекта (внешний цикл сохраняет BC). Адрес MAP_BUFFER+Y*32+X собирается
; из битов координат без пяти 16-битных сдвигов и сохранения HL/DE.
; При ВИ53 и буфере: 45 байт, до входа в обработчик 318 тактов в тишине
; без опроса стрелок; до 1831 при смене посылки и опросе. Одно чтение карты,
; две записи переменных и два чтения таблицы; до 25 обращений к оборудованию.
fast_cell_update:
    IF ALMAZ_PIT
        CALL audio_update
    ENDIF
    IF ALMAZ_OPTIMIZE
        CALL input_service
    ENDIF
    LD A,C
    RRCA
    RRCA
    RRCA
    LD L,A
    AND 1
    OR HIGH(MAP_BUFFER)
    LD H,A
    LD A,L
    AND 0E0h
    OR B
    LD L,A
    LD (v_fast_cell_pointer),HL
    LD A,(HL)
    LD E,A
    AND 0F0h
    LD (v_cell_flags),A
    LD A,E
    AND 00Fh
    ADD A
    LD HL,t_fast_handlers
    ADD A,L
    LD L,A
    LD E,(HL)
    INC HL
    LD D,(HL)
    EX DE,HL
    JP (HL)

; Вход B/C=клетка, v_cell_flags=её старшая тетрада. AF/DE/HL разрушаются,
; BC сохраняется на быстром пути. При ненулевом состоянии выполнить
; исходную физику, в том числе обработку уже перемещённого объекта.
; Неподвижный камень: 14 байт, 172 такта, три чтения карты, без записей/ВВ.
fast_rock_update:
    LD A,(v_cell_flags)
    OR A
    JP NZ,rock_update
    CALL fast_supported
    JP Z,rock_check_below
fast_no_update:
    RET

fast_diamond_update:
    LD A,(v_cell_flags)
    OR A
    JP NZ,diamond_update
    CALL fast_supported
    JP Z,diamond_check_below
    LD A,5
    IF ALMAZ_OPTIMIZE
        JP animation_static_draw
    ELSE
        JP tile_draw
    ENDIF

; Вход v_fast_cell_pointer=адрес текущей неподвижной клетки.
; Выход Z: хотя бы одна из трёх нижних клеток пуста, нужна исходная физика.
; NZ: все заняты, падение и оба перекатывания невозможны. BC сохраняется;
; AF/DE/HL разрушаются. v_cell_flags остаётся состоянием текущего объекта.
; Только чтение карты: три чтения, ни записи, ни обращения к оборудованию.
; Старшие тетрады соседей игнорируются, как в исходном map_get_cell:
; дым с типом 0 по-прежнему считается свободной клеткой для гравитации.
; 21 байт, 108 тактов при трёх занятых клетках (обычный и худший путь).
fast_supported:
    LD HL,(v_fast_cell_pointer)
    LD DE,31
    ADD HL,DE
    LD A,(HL)
    AND 00Fh
    RET Z
    INC HL
    LD A,(HL)
    AND 00Fh
    RET Z
    INC HL
    LD A,(HL)
    AND 00Fh
    RET

; Низшая тетрада задаёт один из 16 адресов; таблица не пересекает страницу.
    ALIGN 32, 0FFh
t_fast_handlers:
    DW smoke_update, exit_update, fast_no_update, fast_no_update
    DW fast_rock_update, fast_diamond_update, square_update, butterfly_update
    DW fungus_update, player_update, fast_no_update, fast_no_update
    DW fast_no_update, fast_no_update, fast_no_update, fast_no_update
    ASSERT LOW(t_fast_handlers) <= 0E0h
v_fast_cell_pointer: DW MAP_BUFFER
