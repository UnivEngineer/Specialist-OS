; Измерительный сеанс ВИ53. Перед настройкой получите TIMER_RAW.
; Процедуры настройки размещаются в доступном для записи коде приложения.
; TIMING_PIT_WRITES_ONLY оставляет привязку и запись отдельных регистров.
    INCLUDE "../abi/timer.inc"

;----------------------------------------------------------------------------
; MXOS
;
; Подготовить четыре процедуры записи регистров на текущей платформе.
;
; вход:
;   ВИ53 доступен через io_pitGetPort. Вызывающий владеет таймером.
;
; выход:
;   Настроены pitTimerWriteCh0/Ch1/Ch2/Control. A, BC, DE, HL и флаги меняются.
;   Привязка не обращается к аппаратным регистрам. Повторная привязка допустима.
;----------------------------------------------------------------------------

pitTimerBindWrites:
    call io_pitGetPort
    ld a,0D3h                 ; OUT n, NOP при H=0.
    jp c,.bind
    ld a,032h                 ; STA nn.
.bind:
    ld de,pitTimerWriteCh0
    ld b,PIT_CONTROL+1
.next:
    ld (de),a
    inc de
    ex de,hl
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    inc hl                   ; Пропустить RET.
    ex de,hl
    inc hl
    dec b
    jp nz,.next
    ret

;----------------------------------------------------------------------------
; MXOS
;
; Записать A в указанный регистр привязанного таймера (канал 0/1/2 или управление).
;
; вход:
;   A = байт записи. Вызвана pitTimerBindWrites или pitTimerInit.
;
; выход:
;   Все регистры и флаги сохранены. Аппаратура принимает байт через STA или OUT.
;----------------------------------------------------------------------------

pitTimerWriteCh0:     DB 0, 0, 0, 0C9h
pitTimerWriteCh1:     DB 0, 0, 0, 0C9h
pitTimerWriteCh2:     DB 0, 0, 0, 0C9h
pitTimerWriteControl: DB 0, 0, 0, 0C9h
    ASSERT pitTimerWriteCh1-pitTimerWriteCh0 == 4
    ASSERT pitTimerWriteCh2-pitTimerWriteCh0 == 8
    ASSERT pitTimerWriteControl-pitTimerWriteCh0 == 12

    IFNDEF TIMING_PIT_WRITES_ONLY
;----------------------------------------------------------------------------
; MXOS
;
; Настроить каскад CLK1 / 256 -> OUT1/CLK2 / 65536 (pitTimerInit).
; pitTimerInitDivisor использует переданный в HL делитель канала 1.
;
; вход:
;   Таймер захвачен. Для pitTimerInitDivisor HL = двоичный делитель 2..65535
;   (0 означает 65536). Требуются GATE1=GATE2=1 и связь OUT1 с CLK2.
;   Для программной проверки старта при 2 МГц подходят делители до 4096.
;
; выход:
;   CF=0, ZF=0: счёт начался. CF=1, ZF=1: движение не обнаружено.
;   BC и DE сохранены. A, HL и остальные флаги меняются.
;----------------------------------------------------------------------------

pitTimerInit:
    ld hl,256
pitTimerInitDivisor:
    push bc
    push de
    push hl
    call pitTimerBindWrites
    pop bc
    ld a,036h
    call pitTimerWriteControl
    ld a,074h
    call pitTimerWriteControl
    ld a,c
    call pitTimerWriteCh1
    ld a,b
    call pitTimerWriteCh1
    ld a,0B4h
    call pitTimerWriteControl
    xor a
    call pitTimerWriteCh2
    call pitTimerWriteCh2
    pop de
    pop bc

;----------------------------------------------------------------------------
; MXOS
;
; Дождаться двух изменений счётчика 2 после смены конфигурации. Первая
; загрузка счётчика идёт по CLK, поэтому немедленное чтение бывает прежним.
;
; вход:
;   ВИ53 настроен на последовательное чтение младшего и старшего байтов.
;
; выход:
;   CF=0, ZF=0: готовность. CF=1, ZF=1: счётчик стоял 256 проверок.
;   BC и DE сохранены. A, HL и остальные флаги меняются.
;----------------------------------------------------------------------------

pitTimerWaitReady:
    push bc
    push de
    ld bc,0200h
    call io_pitRead
    ld e,l
.wait:
    call io_pitRead
    ld a,l
    sub e
    jp z,.next
    ld e,l
    dec b
    jp z,.ready
.next:
    dec c
    jp nz,.wait
    scf
    jp .done
.ready:
    or a
.done:
    pop de
    pop bc
    ret
    ENDIF
