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

; HL=защёлкнутый убывающий счётчик. AF/HL разрушены, BC/DE сохранены.
; 14 байт, 66 тактов без входного JP/CALL. 8253, без read-back 8254.
pitTimerRead:
            ld      a,080h
            ld      (IO_TIMER_MODE),a
            ld      a,(IO_TIMER_CH2)
            ld      l,a
            ld      a,(IO_TIMER_CH2)
            ld      h,a
            ret
