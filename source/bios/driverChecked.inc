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
            ; В STD F000h–F7FFh отображаются устройства, а не RAM.
            ; Сектор EF00h ещё помещается целиком; EF01h уже пересекает границу.
            cp      0EFh
            jp c,   romStdCheckEnd
            jp nz,  romStdReadLow
            ld      a, l
            or      a
            jp nz,  romStdReadLow
romStdCheckEnd:
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
            jp      romBufferedRead

ramDRead:
            call    ramDValidate
            jp c,   popDBHA_error
            call    ramDReadSector
            jp      popDBHA_success

;----------------------------------------------------------------------------
; Запись блока RAM-диска.

diskDriver_w:
            or      a
            jp z,   diskWriteFailed

            call    ramDValidate
            jp c,   diskWriteFailed

            call    ramDWriteSector
            ; bios_RAMDWrite возвращает основную страницу перед RET.

popDBHA_success:
            pop     de
            pop     bc
            pop     hl
            pop     af
            or      a
            ret

diskWriteFailed:
            ld      a, 1
            ld      (bios_upper.diskWriteErr), a
            jp      popDBHA_error

; Вход DE=сектор, HL=буфер. CF=1 — недопустимый диапазон.
; AF/флаги разрушаются, остальные регистры сохраняются.
; Хвост xxFFh содержит служебный код, общее окно и оборудование.
ramDValidate:
            ld      a, (bios_upper.ramPageCount)
            cp      d
            jp z,   ramDInvalid
            ret c
            ld      a, e
            cp      0FFh
            jp z,   ramDInvalid
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

;----------------------------------------------------------------------------
; Размер накопителя.

diskDriver_s:
            ld      a, (v_drive)
            or      a
            jp z,   diskDriver_s_rom

            ; Геометрию B: публикует реестр DOS/драйвер, не детектор банков BIOS.
            ; До запуска DOS встроенный драйвер сообщает размер только ROM A:.
            pop     af
            scf
            ret

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
