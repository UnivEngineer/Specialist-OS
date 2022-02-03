;+---------------------------------------------------------------------------
; MXOS
; Драйвер диска в ДОЗУ, ПЗУ
;
; Запись 256-байтного блока
;   На входе
;     e = 1
;     d = номер блока 
;     hl = адрес буфера в памяти
;   На выходе
;     все регистры сохраняются
;
; Чтение 256-байтного блока
;   На входе
;     e = 2
;     d = номер блока 
;     hl = адрес буфера в памяти
;   На выходе
;     все регистры сохраняются
;
; Получение размера накопителя
;   На входе
;     e = 3
;   На выходе
;     a = кол-во секторов на диске (для функции e=0)
;     bc, de, hl - сохраняются
;
; 2013-12-12 Дизассемблировано и доработано vinxru
; 2022-02-02 Доработано SpaceEngineer
;----------------------------------------------------------------------------

    IF ROM_64K
ROM_SIZE  =  0 
    ELSE
ROM_SIZE  =  0C0h 
    ENDIF

diskDriver: ; Сохраняем регистры
            push    hl
            push    bc          ; +1
            push    de
            push    af

            ; Заранее загружаем в С активный диск
            ld      a, (v_drive); +4
            ld      c, a        ; +5

            ; Функция 1 - Запись блока
            dec     e
            jp z,   diskDriver_w

            ; Функция 2 - Чтение блока
            dec     e
            jp z,   diskDriver_r
            
            ; Функция 3 - Получение размера диска
            pop     af          
            inc     c
            ld      a, ROM_SIZE
            jp z,   popDBh_ret  ; Восстановливаем регистры и выходим
            ld      a, 0FFh     ; Размер RAM-диска            
            jp      popDBh_ret  ; Восстановливаем регистры и выходим
            
; ---------------------------------------------------------------------------
; Функция 2 - Чтение блока

diskDriver_r:
            ; Это ROM-диск?
            or      a
            jp nz,  ramDRead ; нет, переходим к драйверу RAM-диска

    IF ROM_64K

            ; Это старшая часть ПЗУ?
            ld      a, 80h
            xor     d
            jp m,   romMXRead; нет переходим к драйверу ROM-диска MX (Специалист-MX2)

            ; Драйвер ROM-диска STD (Специалист-MX2)
            ld      d, a
            inc     de ; пропускаем первые 4 байта в ПЗУ STD (это код перехода на загрузчик)
            inc     de
            inc     de
            inc     de
            ; Включаем ПЗУ STD (да, надо регистр цвета сбросить для этого)
            xor     a
            ld      (IO_COLOR), a
            ; Включаем режим STD
romStdReadLoop:
            ld      (IO_PAGE_STD), a
            ; Чтение
            ld      a, (de)
            ; Включение ОЗУ MX
            ld      (IO_PAGE_RAM-800h),a
            ; Запись
            ld      (hl), a
            inc     hl
            ; Цикл
            inc     de
            ld      a, 4
            cp      e
            jp nz,  romStdReadLoop

            ; Восстаналиваем цвет
            ld      a, COLOR_BIOS
            ld      (IO_COLOR), a
            ; Выходим
            jp      popa_adh_ret
            
    ENDIF

romMXRead:
romMXReadDrv:
            ld      a, c    ; номер страницы = 0
romMXReadLoop:
            ld      (IO_PAGE_ROM), a
            ld      a, (de)
            ld      (IO_PAGE_RAM), a
            ld      (hl), a
            inc     hl
            inc     e
            jp nz,  romMXReadDrv
            jp      popa_adh_ret

ramDRead:
            ; Вызываем драйвер чтения RAM-диска
            call    bios_RAMDRead

popa_adh_ret:
            ; Восстанавливаем регистры и выходим
            pop     af
            jp      popDBh_ret

; ---------------------------------------------------------------------------
; Функция 1 - Запись блока

diskDriver_w:
            ; Это ROM-диск?
            or      a
            jp z,   popa_adh_ret    ; да, восстанавливаем регистры и выходим

            ; Вызываем драйвер чтения RAM-диска
            call    bios_RAMDWrite

            ; Восстановливаем регистры и выходим
            jp      popa_adh_ret

