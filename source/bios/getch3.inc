;----------------------------------------------------------------------------
; MXOS
; Ожидание ввода с клавиатуры (продолжение)
;
; На выходе
;  ? - код
;
; 2013-12-12 Дизассемблировано vinxru
;----------------------------------------------------------------------------

getch3:     ; Shift+РУС/LAT меняет регистр, но не раскладку знакогенератора.
            ; KOI выбирает RUN/программа явным bios_setFontEncoding.
getch_capsLock:
            ; Переключаем capsLock
            ld      a, (bios_vars.keyLocks)
            xor     80h
            ld      (bios_vars.keyLocks), a

            ; Меняем частоту
            ld      a, (bios_vars.beepFreq)
            xor     10h
            ld      (bios_vars.beepFreq),a

            ; Чтобы был двойной звуковой сигнал, на строй и новой частоте
            call    beep

            ; Ждем следующую клавишу
            jp      getch_retry

; ---------------------------------------------------------------------------
; CAPS LOCK для KOI-7 или РУС/LAT для KOI-8

getch_rc:   ; В режиме KOI-8 мы переключаем кодировку
            ld    a, (bios_upper.koi7)
            inc   a

            ld    a, (bios_vars.keyLocks) ; Требуется в getch_RusLat
            jp nz,  getch_RusLat

            ; В режиме KOI-7 мы перключаем регистр
            jp    getch_capsLock    

; ---------------------------------------------------------------------------

loc_C5A8:   ; Включен KOI-7
            ld    b, a

            ld    a, (IO_KEYB_B)
            and   2
            jp nz,  loc_C311

            ld    a, 0A0h
            xor   b
            ld    b, a

            jp    loc_C311

