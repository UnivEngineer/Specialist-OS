;----------------------------------------------------------------------------
; MXOS
; Ожидание ввода с клавиатуры (продолжение)
;
; На выходе
;  ? - код
;
; 2013-12-12 Дизассемблировано vinxru
;----------------------------------------------------------------------------

loc_C55C:   ; Если нажат лишь РУС/LAT - то обрабатываем его как CAPS/LOCK
            call  setKeybMode91
            ld    a, 0F8h
            ld    (IO_KEYB_B),a
            ld    a, (IO_KEYB_A)           
            push  af
            call  setKeybMode82
            pop   af
            and   8
            jp nz,  getch_capsLock

            ; Меняем KOI-7 / KOI-8
            ld    a, (bios_vars.koi8)
            CPL
            ld    (bios_vars.koi8),a

            ; Звуковой сигнал
            call  beep

            ld    a, 3Ah
            ld    (bios_vars.keyLocks),a

            ; Меняем частоту
            ld    a, 5Fh
            ld    (bios_vars.beepDuration),a

            jp    getch_retry

; ---------------------------------------------------------------------------
; CAPS LOCK для KOI-7

getch_capsLock:   ; Переключаем capsLock
            ld    a, (bios_vars.keyLocks)
            xor   80h
            ld    (bios_vars.keyLocks),a

            ; Изменяем высоту звуковго сигнала        
            ld    a, (bios_vars.beepDuration)
            xor   10h
            ld    (bios_vars.beepDuration),a

            ; Ждем следующую клавишу
            jp    getch_retry

; ---------------------------------------------------------------------------
; CAPS LOCK для KOI-7 или РУС/LAT для KOI-8

getch_rc:   ; В режиме KOI-8 мы переключаем кодировку
            ld    a, (bios_vars.koi8)
            inc   a

            ld    a, (bios_vars.keyLocks) ; Требуется в getch_rusLat
            jp nz,  getch_rusLat

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

