; Цветовой адаптер тайлов. Вход: A=тип; AF/HL/DE сохраняются до
; возвращения в общий рисователь, BC не меняется; флаги сохраняются.
tile_draw_color:
    PUSH AF
    AND 00Fh
    PUSH AF
    PUSH HL
    LD HL,t_tile_colors
    ADD A,L
    LD L,A
    LD A,(HL)
    IF ALMAZ_COLOR == 2
    PUSH AF
    LD A,082h
    LD (0FF03h),A
    POP AF
    ENDIF
    LD (app_color_port),A
    POP HL
    POP AF
    JP tile_draw_pixels

; Выравнивание гарантирует отсутствие переноса при индексе 0..15.
    ALIGN 16, 0FFh
t_tile_colors:
    IF ALMAZ_COLOR == 2
    DB 0D0h, 0C0h, 000h, 000h ; Пустота, выход, кирпичи, граница.
    DB 050h, 080h, 010h, 000h ; Камни, алмазы, квадратики, бабочки.
    DB 040h, 010h, 090h, 000h ; Грибы, игрок, земля, дым.
    DB 0D0h, 0C0h, 000h, 000h
    ELSE
    DB 000h, 010h, 048h, 070h ; Пустота, выход, кирпичи, неразрушаемая стена.
    DB 040h, 0B0h, 0E4h, 0F0h ; Камни, алмазы, квадратики, бабочки.
    DB 050h, 0A0h, 020h, 080h ; Грибы, игрок (ярко-зелёный), земля, дым.
    DB 000h, 010h, 048h, 070h
    ENDIF
    ASSERT LOW(t_tile_colors) <= 0F0h

; Печать цветом рамки (70h на MX, 00h на 8-цветном контроллере):
; соседние пиксели в общем экранном байте
; не перекрашиваются другим цветом. Собственные AF сохраняются;
; остальные регистры, флаги и HL строки — по контракту BIOS.
hud_print_string:
    PUSH AF
    IF ALMAZ_COLOR == 2
    LD A,082h
    LD (0FF03h),A
    ENDIF
    LD A,app_hud_color
    LD (app_color_port),A
    POP AF
    JP app_bios_print_string
hud_print_char:
    PUSH AF
    IF ALMAZ_COLOR == 2
    LD A,082h
    LD (0FF03h),A
    ENDIF
    LD A,app_hud_color
    LD (app_color_port),A
    POP AF
    JP app_bios_print_char
