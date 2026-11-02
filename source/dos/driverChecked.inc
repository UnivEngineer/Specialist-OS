;+---------------------------------------------------------------------------
; Встроенный драйвер ROM/RAM-диска.
;
; Для функций 1/2:
;   CF=0 - операция выполнена;
;   CF=1 - ошибка или неподдерживаемая функция.
; Все входные регистры сохраняются. Встроенный RAM-диск не выполняет
; повторное чтение: для ОЗУ оно только удваивает объём обмена. Flash-драйверы
; ожидают готовность микросхемы без полного сравнения записанных данных.
;----------------------------------------------------------------------------

diskDriver:
            push    af

            ld      a, b
            inc     a
            jp z,   diskDriverInfo
            dec     a
            cp      3
            jp z,   diskDriver_s

            push    hl
            push    bc
            push    de

            ld      a, (v_drive)
            ld      c, a

            dec     b
            jp z,   diskDriver_w
            dec     b
            jp z,   diskDriver_r

popDBHA_error:
            pop     de
            pop     bc
            pop     hl
            pop     af
            scf
            ret

;----------------------------------------------------------------------------
; Чтение блока.

diskDriver_r:
            or      a
            jp nz,  ramDRead
    IF ROM_64K == 0
            jp      romBankRead
    ENDIF

    IF ROM_64K
            ; Том от 128 КБ использует линейные MX-банки, а не STD-половину MX2.
            ld      a, (v_diskInfo.totalSectors + 1)
            cp      2
            jp nc,  romBankRead
            ld      a, 080h
            xor     e
            jp m,   romMXRead

            ld      d, a
            ld      e, 0
            inc     de
            inc     de
            inc     de
            inc     de

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
            ld      (IO_PAGE_STD), a
            ld      a, (de)
            ld      (IO_PAGE_RAM-800h), a
            ld      (hl), a
            inc     hl
            inc     de
            ld      a, 4
            cp      e
            jp nz,  romStdReadLoop

romStdReadExit:
            ld      a, COLOR_BIOS
            ld      (IO_COLOR), a
            jp      popDBHA_success
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
            ld      d, e
            ld      e, 0
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
            jp      popDBHA_success

ramDRead:
            call    ramDValidate
            jp c,   popDBHA_error
    IF RAMD_BUFFERED
            call    ramDByteRead
    ELSE
            ld      c, d
            ld      d, e
            ld      e, 0
            call    bios_RAMDRead
    ENDIF
            jp      popDBHA_success

;----------------------------------------------------------------------------
; Запись блока RAM-диска.

diskDriver_w:
            or      a
            jp z,   diskWriteFailed

            call    ramDValidate
            jp c,   diskWriteFailed

    IF RAMD_BUFFERED
            call    ramDByteWrite
    ELSE
            ld      c, d
            ld      d, e
            ld      e, 0
            call    bios_RAMDWrite
    ENDIF
            ; bios_RAMDWrite возвращает основную страницу перед RET.
            ; Legacy: 70 тактов/байт, 17920 тактов на сектор
            ; (около 9 мс при 2 МГц): 256 чтений ОЗУ, 256 записей
            ; RAM-диска и 512 переключений маппера.

popDBHA_success:
            pop     de
            pop     bc
            pop     hl
            pop     af
            or      a
            ret

diskWriteFailed:
            ld      a, 1
            ld      (bios_vars.diskWriteErr), a
            jp      popDBHA_error

; Вход DE=сектор, HL=буфер. CF=1 — недопустимый диапазон.
; AF/флаги разрушаются, остальные регистры сохраняются.
; Хвост xxFFh содержит служебный код, общее окно и оборудование.
ramDValidate:
            ld      a, (bios_vars.ramPageCount)
            cp      d
            jp z,   ramDInvalid
            ret c
            ld      a, e
            cp      0FFh
            jp z,   ramDInvalid
    IF RAMD_BUFFERED
            ld      a, h
            cp      0FEh
            jp c,   ramDValid
            jp nz,  ramDInvalid
            ld      a, l
            or      a
            ret z
ramDInvalid:
            scf
            ret
ramDValid:
            or      a
            ret
    ELSE
            or      a
            ret
ramDInvalid:
            scf
            ret
    ENDIF

;----------------------------------------------------------------------------
; Размер накопителя.

diskDriver_s:
            ld      a, (v_drive)
            or      a
            jp z,   diskDriver_s_rom

            ld      a, (bios_vars.ramPageCount)
            ld      e, a
            ld      d, 0
            ld      hl, 65536 / SECTOR_SIZE
            call    mul_hl_de
            jp      diskDriver_s_exit

diskDriver_s_rom:
            ld      hl, (v_diskInfo.totalSectors)

diskDriver_s_exit:
            ld      bc, 0
            ex      hl, de
            ld      hl, v_diskInfo
            pop     af
            ret

;----------------------------------------------------------------------------

diskDriverDummy:
            push    af
            ld      a, b
            inc     a
            jp z,   diskDriverNoInfo
            dec     a
            cp      3
            jp nz,  diskDriverDummyExit

            ld      de, 0

diskDriverDummyExit:
            ld      bc, 0
            ld      hl, v_diskInfo
            pop     af
            ret
