; Исходный программный музыкальный проигрыватель для сборки с ВИ53.
; Вход HL=таблица DB период, число повторов; 0=пауза, FFh=конец.
; Выход BC/DE сохраняются; AF/HL меняются. Блокирующий путь вне логики игры.
; Тело и паузы совпадают с историческим sound_sequence/sound_beep;
; адреса переходов и вызовов перенесены, точки BIOS учитывают выбранный ABI.
; Сохранён и общий BIOS-опрос после ноты: его длительность входит в темп.
; 49 байт вместе с music_beep. Нота: 226..230 тактов плюс тела двух
; вызовов BIOS; обычный путь — 230. Конец — 56 тактов, пауза — 98401.
; Внешний CALL music_sequence добавляет 17 тактов. Экран/ВИ53 не затрагивает.
music_sequence:
    LD A,(HL)
    INC HL
    CP 0
    JP NZ,music_test_end
    CALL sound_rest
    JP music_sequence
music_test_end:
    CP 0FFh
    JP NZ,music_play_note
    RET
music_play_note:
    LD (app_beep_frequency),A
    LD A,(HL)
    INC HL
    LD (app_beep_duration),A
    CALL music_beep
music_keycheck:
    CALL app_bios_keycheck
    CP 01Fh
    RET Z
    JP music_sequence

; Один исходный тон через BIOS, BC/DE/HL сохраняются, AF — как у BIOS.
; ВИ53 до входа сюда уже остановлен в audio_music.
music_beep:
    PUSH HL
    PUSH DE
    PUSH BC
    CALL app_bios_beep
    POP BC
    POP DE
    POP HL
    RET
