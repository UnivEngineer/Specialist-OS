;+---------------------------------------------------------------------------
; MXOS
; Чтение байта с ленты
;
; На входе
;  a=255 пропуск пилот-тона до байта до E6h, загрузка первого байта после
;  a=8   загрузка байта (8 бит)
;
; На выходе
;  c - байт
;  de, hl - сохраняются
;
; 2013-12-12 Дизассемблировано vinxru
;----------------------------------------------------------------------------

tapeRead:   ; Сохраняем регистры
            push  bc
            push  de

            ; Тут будет принятый байт
            ld    c, 0

            ; d=8 если загрузка (счетчик бит) или 0FFh если ожидание
            ld    d, a

tapeRead_0: ; Получаем состояние
            ld    a, (IO_KEYB_B)
            and   1
            ld    e, a

            ; Сдвигаем c
            ld    a, c
            and   7Fh
            rlca
            ld    c, a

tapeRead_1: ; Получаем состояние
            ld    a, (IO_KEYB_B)

            ; Если нажата клавиша, выходим
            cp    80h
            jp c, tapeReadError

            ; Ждем изменения сигнала
            and   1
            cp    e
            jp z, tapeRead_1

            ; Сохраняем бит
            or    c
            ld    c, a

            ; Задержка
            call  readDelay

            ld    a, (IO_KEYB_B)
            and   1
            ld    e, a

            ; Если происходит загрузка данных, переходим на tapeRead_4
            ld    a, d
            or    a
            jp p, tapeRead_4

            ; Если происходит ожидание
            ; Если не найден 0E6h, переходим tapeRead_2
            ld    a, c        
            cp    0E6h
            jp nz,  tapeRead_2

            ; Начинаем загрузку без инверсии
            xor   a
            ld    (bios_vars.tapeInverse),a
            jp    tapeRead_3

tapeRead_2: ; Если не найден 19h, переходим на tapeRead_0
            cp    19h
            jp nz,  tapeRead_0

            ; Начинаем загрузку с инверсией
            ld    a, 0FFh
            ld    (bios_vars.tapeInverse),a

            ; Загружаем 8 бит
tapeRead_3: ld    d, 9

            ; Повторяем 8 байт
tapeRead_4: dec   d
            jp nz,  tapeRead_0

            ; Инверсия байта
            ld    a, (bios_vars.tapeInverse)
            xor   c

            ; Восстаналиваем регистры
            pop   de
            pop   bc
            ret

