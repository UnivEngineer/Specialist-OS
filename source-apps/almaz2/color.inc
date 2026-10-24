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
    LD (IO_COLOR),A
    POP HL
    POP AF
    JP tile_draw_pixels

; Выравнивание гарантирует отсутствие переноса при индексе 0..15.
    ALIGN 16, 0FFh
t_tile_colors:
    DB 000h, 010h, 048h, 070h ; Пустота, выход, кирпичи, неразрушаемая стена.
    DB 040h, 0B0h, 0E4h, 0F0h ; Камни, алмазы, квадратики, бабочки.
    DB 050h, 0A0h, 020h, 080h ; Грибы, игрок (ярко-зелёный), земля, дым.
    DB 000h, 010h, 048h, 070h
    ASSERT LOW(t_tile_colors) <= 0F0h

; Печать цветом рамки (70h): соседние пиксели в общем экранном байте
; не перекрашиваются другим цветом. Собственные AF сохраняются;
; остальные регистры, флаги и HL строки — по контракту BIOS.
hud_print_string:
    PUSH AF
    LD A,070h
    LD (IO_COLOR),A
    POP AF
    JP app_bios_print_string
hud_print_char:
    PUSH AF
    LD A,070h
    LD (IO_COLOR),A
    POP AF
    JP app_bios_print_char
