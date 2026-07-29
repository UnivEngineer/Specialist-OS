;----------------------------------------------------------------------------
; MXOS
; Ожидание ввода с клавиатуры
;
; На выходе
;  ? - код
;
; 2013-12-12 Дизассемблировано vinxru
;----------------------------------------------------------------------------

getch2:     ; Сохраняем регистры
            push    hl
            push    bc
            push    de

            ; Получаем код нажатой клавиши и сохраняем его в bios_vars.lastKey
            call    keyScan2
            ld      (bios_vars.lastKey),a

getch_retry:
            ; Тут будет признак, нарисован курсор или нет
            ld      c, 1

loc_C227:   ; Рисуем курсор
            call    drawCursor2

            ; Задержка
            ld      a, (bios_vars.cursorDelay)
loc_C22D:   ld      b, 40h
            call    delay_b
            dec     a
            jp nz,  loc_C22D

loc_C236:   ; Рисуем/стираем курсор
            call    drawCursor
            
            ; Задержка мерцания курсора
            ld      de, 500h

loc_C23C:   ; Получаем код нажатой клавиши
            call    keyScan2
            ;cp      80h         ; х.з.
            cp      0FFh
            jp nz,  getch2_pressed

            ; Записываем bios_vars.lastKey=0FFh
            ld      (bios_vars.lastKey),a

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

setKeybMode82:
            ld      a, 82h
            ld      (IO_KEYB_MODE),a
            ret

; ---------------------------------------------------------------------------

setKeybMode91:
            ld      a, 91h
            ld      (IO_KEYB_MODE),a
            ret

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
            ld      hl, (bios_vars.lastLastKey)   ; h = bios_vars.lastKey, l = bios_vars.lastLastKey
            ld      a, b        ; a = b = код нажатой клавиши
            cp      h
            jp nz,    loc_C28D    ; Код нажатой клавиши изменился
            cp      l
            jp z,   loc_C295

            ; Рисуем курсор
            call    drawCursor

            ; Ждем, пока отпустят клавишу
            ld      a, (bios_vars.antiBsDelay)
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
            call    beep

loc_C290:   ld      l, h
            ld      h, b  ; код нажатой и предыдущей клавиши сохраняем в bios_vars.lastKey
            ld      (bios_vars.lastLastKey), hl

loc_C295:   ; Не служебные клавиши CAPS LOCK, SHIFT и РУС/LAT не влияют
            ld      a, b
            cp      21h
            jp c,   getch_noShift   
            ; На клавиши c кодами 21h-3Fh влияет SHIFT
            cp      40h
            jp c,   getch_shift 
            ; На символьные клавиши (40h-7Eh) влияют CAPS LOCK, SHIFT и РУС/LAT
            cp      7Fh
            jp c,   getch_chars
            ; Не служебные клавиши (7Fh+) CAPS LOCK, SHIFT и РУС/LAT не влияют
            jp z,   getch_noShift

            ; ...
            ;nop
            ;nop

            ; Переходим, если код клавиши не 81h
            cp      81h
            jp nz,  getch_noShift

            ; Код клавиши 81h

            ; Если шифт не нажат
            ld      a, (IO_KEYB_B)
            and     2
            jp nz,  getch3

            ; Код клавиши 81h с шифтом

            ; Звуковой сигнал
            call    beep

            ; CAPS LOCK для KOI-7 или РУС/LAT для KOI-8
            jp      getch_rc

; ---------------------------------------------------------------------------
; Нажата комбинация Shift + РУС/LAT

getch_RusLat:
            ; на входе a = bios_vars.keyLocks
            xor     81h ; инверируем сарший и младший биты
            ld      (bios_vars.keyLocks), a

            ; Особый звуковой сигнал
            ld      a, (bios_vars.beepFreq)
            push    af
            ld      a, 4Fh
            ld      (bios_vars.beepFreq), a
            call    beep
            ld      a, 5Fh
            ld      (bios_vars.beepFreq), a
            call    beep
            pop     af
            ld      (bios_vars.beepFreq), a

            ; ...
            ;nop

            jp      getch_retry

; ---------------------------------------------------------------------------
; Включаем кирилицу или строчные буквы

getch_chars:
            ; Если не нажат CAPS LOCK, пропускаем код ниже
            ld      a, (bios_vars.keyLocks)
            ld      c, a
            and     a
            jp M,   loc_C2EA    ; CAPS LOCK

            ; Превращаем заглавные в строчные
            ld      a, b
            xor     20h
            ld      b, a

loc_C2EA:   ; Если зафиксирвоан ли РУС/ЛАТ, помещаем в c=1
            ld      a, c
            and     1
            ld      c, a

            ; Если шифт нажат, помещаем в a=0
            ld      a, (IO_KEYB_B)
            and     2
            rrca    ; ! Как на счет флага c ?

            ; Превращаем английские символы в русские если a^c==0
            xor     c
            ld      a, b
            jp nz,  loc_C5A8
            add     80h
            jp      loc_C5A8

; ---------------------------------------------------------------------------
; Меняем цифры на символы

getch_shift:
            ; Если не нажат шифт, пропускаем код ниже
            ld      a, (IO_KEYB_B)
            and     2
            ld      a, b
            jp nz,  loc_C5A8

            ; Меняем цифры на символы
            xor     10h

            ; Одно исключение 30h должен меняться на 5Fh
            cp      20h
            jp nz,  getch_noShift
            ld      a, 5Fh

            ; Прододжение getch_noShift

; ---------------------------------------------------------------------------
; Символы без изменений

getch_noShift:
            ; Сохраняем код нажатой клавиши в b     
            ld      b, a

loc_C311:   call    setKeybMode83
            ld      a, 0F7h
            ld      (IO_KEYB_A),a
            ld      a, (IO_KEYB_B)
            cpl
            rrca
            rrca
            rrca
            call    setKeybMode82

            ;nop

            ; Восстаналиваем код нажатой клавиши
            ld      a, b

popa_ret_2: ; Восстаналиваем регистры и выходим
            pop     de
            pop     bc
            pop     hl
            ret

