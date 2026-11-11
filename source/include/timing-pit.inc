; Локальное управление ВИ53 для BIOS и измерительных программ.
; INCLUDE после mxos.inc; процедурные метки pitTimerInit/pitTimerRead.
; Все данные находятся в регистрах CPU и ВИ53, системных переменных нет.
; Работает в MAIN, DOS и EXEC: FFECh..FFEFh доступны в любой странице.
; TIMING_PIT_READ_ONLY исключает настройку (22 байта), если её делает хозяин.

    IFNDEF TIMING_PIT_READ_ONLY
; Общий каскад ВИ53: CLK1 / 256 -> OUT1/CLK2 / 65536.
; Требуются GATE1=GATE2=1 и аппаратное соединение OUT1 с CLK2.
; Это счётчик интервалов, не системные часы; с генерацией звука несовместим.

; Входа нет. Настраивает каналы 1/2 и останавливает несущую канала 0.
; AF, HL и флаги разрушены, BC/DE сохранены. 22 байта.
pitTimerInit:
            ld      hl,IO_TIMER_MODE
            ld      (hl),036h
            ld      (hl),074h
            dec     hl
            dec     hl
            xor     a
            ld      (hl),a
            inc     a
            ld      (hl),a
            inc     hl
            inc     hl
            ld      (hl),0B4h
            dec     hl
            xor     a
            ld      (hl),a
            ld      (hl),a
            ret

    ENDIF

; Локальное чтение ВИ53, одинаковое в MAIN, DOS и EXEC-банке.
; Требуется предварительная настройка каскада через pitTimerInit.
; Аппаратные регистры доступны во всех страницах; переход банка не нужен.
; HL=защёлкнутый убывающий счётчик. AF/HL разрушены, BC/DE сохранены.
; 10 байт, 54 такта с RET, без входного CALL. 8253, без read-back 8254.
pitTimerRead:
            ld      hl,IO_TIMER_MODE
            ld      (hl),080h
            dec     hl
            ld      a,(hl)
            ld      h,(hl)
            ld      l,a
            ret
