;+---------------------------------------------------------------------------
; MXOS
; Встроенный драйвер ROM-диска (в ПЗУ) и RAM-диска (в ДОЗУ)
;
; Запись 256-байтного блока
;   На входе
;     b = 1                 // было: e = 1
;     de = номер кластера   // было: d = номер блока 
;     hl = адрес буфера в памяти
;   На выходе
;     все регистры сохраняются
;
; Чтение 256-байтного блока
;   На входе
;     b = 2                 // было: e = 2
;     de = номер кластера   // было: d = номер блока 
;     hl = адрес буфера в памяти
;   На выходе
;     все регистры сохраняются
;
; Получение размера накопителя
;   На входе
;     b = 3                 // было: e = 3
;   На выходе
;     de = количество секторов на диске (0 - нет драйвера)
;     hl = адрес структуры DISK_INFO
;
; 2013-12-12 Дизассемблировано и доработано vinxru
; 2022-02-15 Доработано SpaceEngineer
;----------------------------------------------------------------------------

diskDriver: ; Сохраняем регистры
            push    af

            ; Функция 3 - Получение размера диска
            ld      a, b
            inc     a
            jp z,   diskDriverInfo
            dec     a
            cp      3
            jp z,   diskDriver_s

             ; Сохраняем регистры
            push    hl
            push    bc
            push    de

            ; Заранее загружаем в С активный диск
            ld      a, (v_drive)
            ld      c, a

            ; Функция 1 - Запись блока
            dec     b
            jp z,   diskDriver_w

            ; Функция 2 - Чтение блока
            dec     b
            jp z,   diskDriver_r

popDBHA_ret:
            ; Восстанавливаем регистры и выходим
            pop     de
            pop     bc
            pop     hl
            pop     af
            ret

            
; ---------------------------------------------------------------------------
; Функция 2 - Чтение блока

diskDriver_r:
            ; Это ROM-диск?
            or      a
            jp nz,  ramDRead ; нет, переходим к драйверу RAM-диска
    IF ROM_64K == 0
            jp      romBankRead
    ENDIF

    IF ROM_64K
            ; Том от 128 КБ использует линейные MX-банки, а не STD-половину MX2.
            ld      a, (v_diskInfo.totalSectors + 1)
            cp      2
            jp nc,  romBankRead

            ; Это старшая часть ПЗУ?
            ld      a, 80h
            xor     e
            jp m,   romMXRead; нет переходим к драйверу ROM-диска MX (Специалист-MX2)

            ; Драйвер ROM-диска STD (Специалист-MX2)
            ld      d, a
            ld      e, 0
            inc     de ; пропускаем первые 4 байта в ПЗУ STD (это код перехода на загрузчик)
            inc     de
            inc     de
            inc     de

            ; Включаем ПЗУ STD (да, надо регистр цвета сбросить для этого)
            xor     a
            ld      (IO_COLOR), a
            ld      a, h
            or      a
            jp p,   romStdReadLow
            ; Последний STD-сектор неполон: 7F04h..8003h. Сохраняем
            ; прежнее побайтовое поведение и на границе отображения.
            ld      a, d
            cp      07Fh
            jp nz,  romStdMappedRead
romStdReadLow:
            xor     a
romStdReadLoop:
            ; Включаем режим STD
            ld      (IO_PAGE_STD), a
            ; Чтение
            ld      a, (de)
            ; Включаем ОЗУ MX
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
romStdReadExit:
            ld      a, COLOR_BIOS
            ld      (IO_COLOR), a

            ; Восстанавливаем регистры и выходим
            jp      popDBHA_ret
            
    ENDIF

; Линейный ROM/flash до 512 КБ: банк = DE >> 7, адрес = (E & 7Fh) << 8.
; 14 байт, 57 тактов настройки; горячий цикл ниже остаётся общим.
romBankRead:
            ld      a, e
            add     a, a              ; CF = бит 7 младшего байта сектора
            ld      a, d
            rla
            ld      c, a              ; D0-D3 номера 32-КБ банка для FFFEh
            ld      a, e
            and     07Fh
            ld      d, a
            ld      e, 0
            jp      romMXReadBuffer

romMXRead:
romMXReadDrv:
            ld      d, e    ; d = номер блока 
            ld      e, 0    ; e = 0
                            ; hl = адрес буфера в памяти
romMXReadBuffer:
            ld      a, h
            or      a
            jp m,   romMappedRead
; Нижнее назначение: C=банк, B/A=пара данных. 19 байт, 94 такта/пару.
; На сектор 256 чтений/записей и 256 переключений маппера.
romMXReadLoop:
            ld      a, c
            ld      (IO_PAGE_ROM), a
            ld      a, (de)
            ld      b, a
            inc     e
            ld      a, (de)
            ld      (IO_PAGE_RAM), a
            ld      (hl), b
            inc     hl
            ld      (hl), a
            inc     hl
            inc     e
            jp nz,  romMXReadLoop
            jp      popDBHA_ret

ramDRead:
    IF RAMD_BUFFERED
            call    ramDByteRead
    ELSE
            ; Вызываем драйвер чтения RAM-диска
            ld      c, d        ; c = номер страницы
            ld      d, e        ; d = номер блока 
            ld      e, 0        ; e = 0
                                ; hl = адрес буфера в памяти
            call    bios_RAMDRead

    ENDIF

            ; Восстановливаем регистры и выходим
            jp      popDBHA_ret

; ---------------------------------------------------------------------------
; Функция 1 - Запись блока

diskDriver_w:
            or      a           ; это ROM-диск?
            jp z,   popDBHA_ret ; да, запись не поддерживается

    IF RAMD_BUFFERED
            call    ramDByteWrite
    ELSE
            ; Вызываем драйвер записи RAM-диска
            ld      c, d        ; c = номер страницы
            ld      d, e        ; d = номер блока 
            ld      e, 0        ; e = 0
                                ; hl = адрес буфера в памяти
            call    bios_RAMDWrite

    ENDIF

            ; Восстановливаем регистры и выходим
            jp      popDBHA_ret

; ---------------------------------------------------------------------------
; Функция 3 - Получение размера диска

diskDriver_s:
            ld      a, (v_drive)
            or      a                   ; это ROM-диск?
            jp z,   diskDriver_s_rom    ; да, переходим

            ; Это RAM-диск. Рассчитываем размер, зная количество банок ДОЗУ
            ld      a, (bios_vars.ramPageCount) ; a  = количество 64к банок ДОЗУ
            ld      e, a
            ld      d, 0                        ; de = количество 64к банок ДОЗУ
            ld      hl, 65536 / SECTOR_SIZE     ; hl = количество секторов в одной 64к банке
            call    mul_hl_de                   ; hl = количество секторов в ДОЗУ
            jp      diskDriver_s_exit

diskDriver_s_rom:
            ; У ROM-диска размер должен быть прошит в загрузочном секторе
            ld      hl, (v_diskInfo.totalSectors)

diskDriver_s_exit:
            ld      bc, 0
            ; Возвращаем в de размер диска в секторах
            ex      hl, de

            ; Возвращаем в hl адрес структуры v_diskInfo
            ld      hl, v_diskInfo

            ; Восстановливаем регистры и выходим
            pop     af
            ret

;----------------------------------------------------------------------------
; MXOS
; "Пустой" драйвер диска
; Назначается при старте на накопители C: ... H:
;
; 2022-02-07 SpaceEngineer
;----------------------------------------------------------------------------

diskDriverDummy:
            push    af

            ; Функция 3 - Получение размера диска
            ld      a, b
            inc     a
            jp z,   diskDriverNoInfo
            dec     a
            cp      3
            jp nz,  diskDriverDummyExit

            ld      de, 0   ; вернуть размер 0 - драйвер не установлен

            ; Остальные функции просто ничего не делают
diskDriverDummyExit:
            ld      bc, 0

            ; Возвращаем в hl адрес структуры v_diskInfo
            ld      hl, v_diskInfo

            pop     af
            ret
