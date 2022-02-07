;+---------------------------------------------------------------------------
; MXOS
; Драйверы диска в ДОЗУ (часть, которая копируется на адрес 0FFC0h)
;
; 2013-12-12 Дизассемблировано и доработано vinxru
; 2022-02-02 Доработано SpaceEngineer
;----------------------------------------------------------------------------

;----------------------------------------------------------------------------
; Установить драйвер доступа к ДОЗУ в окно разделяемой памяти
; Специалиста-MX (FFC0h-FFDEh)
;
; вход:
;  b = 0 - установить блочный драйвер, b = 1 - установить побайтовый драйвер
;
; 2022-02-02 SpaceEngineer
;----------------------------------------------------------------------------

setRAMDDriver:
            push  hl
            push  de
            push  bc
            push  af
            dec   b
            jp z, setRAMDDriver_1

            ; Адреса блочного драйвера
            ld    hl, driverBlockStart
            ld    de, driverBlockEnd
            jp    setRAMDDriver_2

setRAMDDriver_1:
            ; Адреса побайтового драйвера
            ld    hl, driverByteStart
            ld    de, driverByteEnd

            ; Копируем драйвер на адрес bc = bios_RAMDRead = 0FFC0h
setRAMDDriver_2:
            ld    bc, bios_RAMDRead
            call  memcpy_bc_hl

            ; Восстанавливаем регистры и выходим
            pop   af
            pop   bc
            pop   de
            pop   hl
            ret

;----------------------------------------------------------------------------
; Блочный драйвер, обрабатывает сразу блок 256 байт
;
; FFC0h - чтение 256-байтного блока
;   вход:
;     c = номер страницы
;     d = номер блока 
;     e = 0
;     hl = адрес буфера в памяти
;
; FFD0h - запись 256-байтного блока
;   вход:
;     c = номер страницы
;     d = номер блока 
;     e = 0
;     hl = адрес буфера в памяти
;
;----------------------------------------------------------------------------

; Чтение 256-байтного блока с диска

driverBlockStart: ; эта п/п копируется на FFC0h
            ld      a, c
            ld      (IO_PAGE_RAMD), a
            ld      a, (de)
            ld      (IO_PAGE_RAM), a
            ld      (hl), a
            inc     hl
            inc     e
            jp nz,  0FFC0h
            ret

            ; осталось места: 1 байт
            nop

; Запись 256-байтного блока на диск

driverBlockWrite: ; эта п/п копируется на FFD0h
            ld      b, (hl)
            ld      a, c
            ld      (IO_PAGE_RAMD), a
            ld      a, b
            ld      (de), a
            ld      (IO_PAGE_RAM), a
            inc     hl
            inc     e
            jp nz,  0FFD0h
            ret

            ; осталось места: 0 байт

driverBlockEnd:

;----------------------------------------------------------------------------
; Побайтовый драйвер, обрабатывает только 1 байт
;
; FFC0h - чтение байта
;   вход:
;     a  = номер страницы
;     hl = адрес байта в странице ДОЗУ
;   выход:
;     c  = считанный байт
;
; FFD0h - запись байта
;   вход:
;     a  = номер страницы
;     hl = байта в странице ДОЗУ
;     c  = записываемый байт
;
;----------------------------------------------------------------------------

driverByteStart: ; эта п/п копируется на FFC0h
            ld      (IO_PAGE_RAMD), a
            ld      c, (hl)
            ld      (IO_PAGE_RAM), a
            ret

            ; осталось места: 8 байт
            nop
            nop
            nop
            nop
            nop
            nop
            nop
            nop

driverByteWrite: ; эта п/п копируется на FFD0h
            ld      (IO_PAGE_RAMD), a
            ld      (hl), c
            ld      (IO_PAGE_RAM), a
            ret

            ; осталось места: 8 байт
            nop
            nop
            nop
            nop
            nop
            nop
            nop
            nop

driverByteEnd:

; ---------------------------------------------------------------------------

; Проверка, что драйверы влезаеют в 32 байта разделяемой памяти Специалиста-MX

    IF driverBlockWrite-driverBlockStart+bios_RAMDRead != bios_RAMDWrite
        ASSERT 0
        DISPLAY /l, "Error! Block briver entry point FFD0 has been shifted: ", driverBlockWrite-driverBlockStart+bios_RAMDRead, " != 0xFFD0"
    ENDIF

    IF driverByteWrite-driverByteStart+bios_RAMDRead != bios_RAMDWrite
        ASSERT 0
        DISPLAY /l, "Error! Byte briver entry point FFD0 has been shifted: ", driverByteWrite-driverByteStart+bios_RAMDRead, " != 0xFFD0"
    ENDIF

    IF driverBlockEnd-driverBlockStart > 32
        ASSERT 0
        DISPLAY /d, "Error! Block briver did not fit: ", driverBlockEnd-driverBlockStart, " > 32 bytes"
    ENDIF

    IF driverByteEnd-driverByteStart > 32
        ASSERT 0
        DISPLAY /d, "Error! Byte briver did not fit: ", driverByteEnd-driverByteStart, " > 32 bytes"
    ENDIF
