;+---------------------------------------------------------------------------
; Встроенный драйвер ROM/RAM-диска.
;
; Для функций 1/2:
;   CF=0 - операция выполнена;
;   CF=1 - ошибка или неподдерживаемая функция.
; Все входные регистры сохраняются. Встроенный RAM-диск не выполняет
; повторное чтение: для ОЗУ оно только удваивает объём обмена. Проверка
; flash и системного ROM выполняется в соответствующих драйверах.
;----------------------------------------------------------------------------

diskDriver:
            push    af

            ld      a, b
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

    IF ROM_64K
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

            ld      a, COLOR_BIOS
            ld      (IO_COLOR), a
            jp      popDBHA_success
    ENDIF

romMXRead:
romMXReadDrv:
            ld      a, c
            ld      d, e
            ld      e, 0
romMXReadLoop:
            ld      (IO_PAGE_ROM), a
            ld      a, (de)
            ld      (IO_PAGE_RAM), a
            ld      (hl), a
            inc     hl
            inc     e
            jp nz,  romMXReadLoop
            jp      popDBHA_success

ramDRead:
            ld      c, d
            ld      d, e
            ld      e, 0
            call    bios_RAMDRead
            jp      popDBHA_success

;----------------------------------------------------------------------------
; Запись блока RAM-диска.

diskDriver_w:
            or      a
            jp z,   diskWriteFailed

            ld      c, d
            ld      d, e
            ld      e, 0
            call    bios_RAMDWrite
            ; bios_RAMDWrite возвращает основную страницу перед RET.
            ; Горячий цикл: 70 тактов/байт, 17920 тактов на сектор
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
            ex      hl, de
            ld      hl, v_diskInfo
            pop     af
            ret

;----------------------------------------------------------------------------

diskDriverDummy:
            push    af
            ld      a, b
            cp      3
            jp nz,  diskDriverDummyExit

            ld      de, 0

diskDriverDummyExit:
            ld      hl, v_diskInfo
            pop     af
            ret
