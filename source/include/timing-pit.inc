; Локальное управление ВИ53 для BIOS и измерительных программ.
; INCLUDE после mxos.inc; локальные процедуры, не фиксированные входы BIOS.
; Все данные находятся в регистрах CPU и ВИ53, системных переменных нет.
; Работает в MAIN, DOS и EXEC: FFECh..FFEFh доступны в любой странице.
; TIMING_PIT_READ_ONLY оставляет только pitTimerRead: каскад уже настроен
; и стабилизирован владельцем; повторно настраивать его в рабочем цикле нельзя.
;
; Обычный запуск: CALL pitTimerInit / JP C,error / CALL pitTimerRead.
; Начальную отметку брать ПОСЛЕ init; ожидание готовности не входит в замер.
; Своя настройка/смена делителя: после записи обоих байтов канала 2 вызвать
; pitTimerWaitReady, проверить CF и только затем читать начальную отметку.
; Запись ВИ53 не загружает счёт немедленно: перенос выполняется по CLK.
; Чтение до него может вернуть прежний счёт и ложный почти полный период.
; timer_probe из timing-counter.inc — отдельная диагностика движения,
; не обязательная задержка между pitTimerInit и первым чтением.

    IFNDEF TIMING_PIT_READ_ONLY
; Общий каскад ВИ53: CLK1 / 256 -> OUT1/CLK2 / 65536.
; Требуются GATE1=GATE2=1 и аппаратное соединение OUT1 с CLK2.
; Это счётчик интервалов, не системные часы; с генерацией звука несовместим.

; Входа нет. Настраивает каналы 1/2 и останавливает несущую канала 0.
; После настройки ждёт готовности через pitTimerWaitReady.
; CF=0: счёт начался; CF=1: не дождались движения (CLK/GATE/делитель).
; AF, HL и флаги разрушены, BC/DE сохранены.
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

; Для уже записанной настройки: дождаться двух изменений канала 2,
; отбросив переход от старого счёта при загрузке и следующий счётный фронт.
; Не перенастраивает ВИ53. CF=0/ZF=0: готов;
; CF=1/ZF=1: тайм-аут после 256 чтений. Движение проверяется по младшему байту.
; AF/HL разрушены, BC/DE сохранены. При CLK=CPU=2 МГц этого достаточно
; для делителей 256 и 4096. Для более медленного CLK/большего делителя
; лимит нужно пересмотреть; успех определяется движением, а не числом NOP.
pitTimerWaitReady:
            push    bc
            push    de
            ld      bc,0200h
            call    pitTimerRead
            ld      e,l
pitTimerWaitLoop:
            call    pitTimerRead
            ld      a,l
            sub     e
            jp z,   pitTimerWaitNext
pitTimerChanged:
            ld      e,l
            dec     b
            jp z,   pitTimerReady
pitTimerWaitNext:
            dec     c
            jp nz,  pitTimerWaitLoop
            scf
            jp      pitTimerWaitDone
pitTimerReady:
            or      a
pitTimerWaitDone:
            pop     de
            pop     bc
            ret

    ENDIF

; Локальное чтение ВИ53, одинаковое в MAIN, DOS и EXEC-банке.
; Требуется успешный pitTimerInit либо своя настройка + pitTimerWaitReady.
; Само чтение не ждёт CLK и не проверяет готовность; BC/DE сохраняются.
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
