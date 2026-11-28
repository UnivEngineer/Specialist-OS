;----------------------------------------------------------------------------
; MXOS
; Ожидание ввода с клавиатуры
;
; выход:
;  ? - код
;
; 2013-12-12 Дизассемблировано vinxru
;----------------------------------------------------------------------------

getch2:     ; Сохраняем регистры
            push    hl
            push    bc
            push    de

            ; Получаем код нажатой клавиши и сохраняем его в mon2_vars.lastKey
            call    keyScan2
            ld      (mon2_vars.lastKey),a

getch_retry:
            ; Тут будет признак, нарисован курсор или нет
            ld      c, 1

loc_C227:   ; Рисуем курсор
            call    drawCursor2

            ; Задержка
            ld      a, (bios_upper.cursorDelay)
loc_C22D:   ld      b, 40h
            call    delay_b
            dec     a
            jp nz,  loc_C22D

loc_C236:   ; Рисуем/стираем курсор
            call    drawCursor
            
            ; Задержка мерцания курсора
            ld      de, 500h

loc_C23C:   call inputIdle
            ; Получаем код нажатой клавиши
            call    keyScan2
            ;cp      80h         ; х.з.
            cp      0FFh
            jp nz,  getch2_pressed

            ; Записываем mon2_vars.lastKey=0FFh
            ld      (mon2_vars.lastKey),a

            ; Повторяем 500h раз
            dec     de
            ld      a, d
            or      e
            jp nz,  loc_C23C

            ; Повтор всего
            jp      loc_C236

; ---------------------------------------------------------------------------

;    DB 0FFh, 0FFh

; ---------------------------------------------------------------------------

; ---------------------------------------------------------------------------
; Была нажата клавиша

getch2_pressed:
            ; Сохраняем код
            ld      b, a

            ; Если нарисован курсор, стираем его
            ld      a, c
            rrca
            jp c,   loc_C269
            call    drawCursor2
loc_C269:   
            ld      hl, (mon2_vars.lastLastKey)   ; h = mon2_vars.lastKey, l = mon2_vars.lastLastKey
            ld      a, b        ; a = b = код нажатой клавиши
            cp      h
            jp nz,    loc_C28D    ; Код нажатой клавиши изменился
            cp      l
            jp z,   loc_C295

            ; Рисуем курсор
            call    drawCursor

            ; Ждем, пока отпустят клавишу
            ld      a, (bios_upper.antiBsDelay)
            ld      d, a
loc_C27C:   call    keyScan2
            cp      h
            jp nz,  loc_C227    ; Быстро нажали другую клавишу
            dec     d
            jp nz,  loc_C27C

            ; Долго держали

            ; Стираем курсор
            call    drawCursor

            jp      loc_C290
; ---------------------------------------------------------------------------

loc_C28D:   ; Звуковой сигнал
            call    biosBeep

loc_C290:   ld      l, h
            ld      h, b  ; код нажатой и предыдущей клавиши сохраняем в mon2_vars.lastKey
            ld      (mon2_vars.lastLastKey), hl

loc_C295:
            call keyDecodeCurrent
            jp nz,popa_ret_2
            ld a,(mon2_vars.keyLocks)
            xor e
            ld a,e
            ld (mon2_vars.keyLocks),a
            jp m,getchCapsBeep
            ld a,(mon2_vars.beepFreq)
            push af
            ld a,04Fh
            ld (mon2_vars.beepFreq),a
            call biosBeep
            ld a,05Fh
            ld (mon2_vars.beepFreq),a
            call biosBeep
            pop af
            ld (mon2_vars.beepFreq),a
            jp getch_retry
getchCapsBeep:
            ld a,(mon2_vars.beepFreq)
            xor 10h
            ld (mon2_vars.beepFreq),a
            call biosBeep
            jp getch_retry

popa_ret_2: ; Восстаналиваем регистры и выходим
            pop     de
            pop     bc
            pop     hl
            ret


; HL=обработчик ожидания или 0. Возврат HL=прежний, AF/DE разрушены.
; Обработчик сохраняет регистры и состояние текстового вывода; не вызывает getch.

;----------------------------------------------------------------------------
; MXOS
;
; Заменить обработчик ожидания ввода и вернуть предыдущий адрес.
;
; вход:
;   hl = новый адрес обработчика, либо 0 для отключения.
;
; выход:
;   inputHook содержит новый адрес; hl = старый адрес, de = переданный новый адрес. bc,
;   a и флаги сохранены.
;----------------------------------------------------------------------------

setInputHook:
            ex de,hl
            ld hl,(inputHook)
            ex de,hl
            ld (inputHook),hl
            ex de,hl
            ret
inputIdle:
            push hl
            ld hl,(inputHook)
            ld a,h
            or l
            call nz,inputIdleCall
            pop hl
            ret
inputIdleCall:
            jp (hl)
inputHook: DW 0
