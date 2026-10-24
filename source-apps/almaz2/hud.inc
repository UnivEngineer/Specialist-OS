; Подложки надписей в верхней рамке карты. Шрифт BIOS имеет размер 6*8;
; координата X курсора задаётся в полупикселях, Y — нижняя строка символа.
; При Y=11 символы занимают строки 4..11. Подложки захватывают 3..12,
; оставляя дополнительный чёрный отступ сверху и снизу.

; Входа нет. Выход HL=SCREEN без HOME. AF/BC/DE и флаги сохраняются.
; Вызывается после полной перерисовки карты, перед выводом SCREEN.
hud_screen_position:
    CALL hud_clear_panels
    LD HL,000Bh
    LD (app_cursor_position),HL
    LD HL,t_screen_text+1
    RET

; Вход C=последняя цифра TIME. Выход и разрушение регистров — как у BIOS.
; Обычный CALL в последней точке вывода сохраняет исходный POP BC/RET.
hud_time_last_char:
    IF ALMAZ_COLOR
        CALL hud_print_char
    ELSE
        CALL app_bios_print_char
    ENDIF
    LD C,020h
    IF ALMAZ_COLOR
        JP hud_print_char
    ELSE
        JP app_bios_print_char
    ENDIF

; Входа нет. Все регистры и флаги сохраняются. Две подложки:
; SCREEN X=32..95, TIME X=160..231; Y=3..12. Цвет текста/рамки 70h
; даёт чёрный фон нулевых байтов и сохраняет цвет соседних пикселей.
; 170 записей экрана; в цветной сборке одна запись цветового регистра.
hud_clear_panels:
    PUSH AF
    PUSH BC
    PUSH DE
    PUSH HL
    IF ALMAZ_COLOR
        LD A,070h
        LD (IO_COLOR),A
    ENDIF
    LD HL,09403h
    LD B,8
    CALL hud_clear_rectangle
    LD HL,0A403h
    LD B,9
    CALL hud_clear_rectangle
    POP HL
    POP DE
    POP BC
    POP AF
    RET

; Вход HL=верхний байт первого столбца, B=число столбцов по 8 пикселей.
; Выход AF/BC/HL разрушаются, DE сохраняется. Высота фиксирована: 10 строк.
hud_clear_rectangle:
    XOR A
hud_clear_column:
    LD C,10
hud_clear_row:
    LD (HL),A
    INC L
    DEC C
    JP NZ,hud_clear_row
    LD L,3
    INC H
    DEC B
    JP NZ,hud_clear_column
    RET
