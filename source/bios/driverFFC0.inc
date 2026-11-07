;+---------------------------------------------------------------------------
; MXOS
; Драйверы диска в ДОЗУ (часть, которая копируется на адрес 0FFC0h)
; Метаданные RAMD (B=0FFh) выдаёт диспетчер C863h, driverInfo.inc.
;
; 2013-12-12 Дизассемблировано и доработано vinxru
; 2022-02-02 Доработано SpaceEngineer
;----------------------------------------------------------------------------

;----------------------------------------------------------------------------
; Установить драйвер доступа к ДОЗУ в окно разделяемой памяти
; Специалиста-MX (FFC0h-FFDEh)
;
; вход:
;  B=RAMD_DRIVER_RESTORE — обычный режим (до RAMDISK.COM — блочный).
;  B=RAMD_DRIVER_BYTE/BLOCK — временный выбор встроенного шаблона.
;  B=RAMD_DRIVER_REGISTER, HL=шаблон — регистрация обычного режима.
;  B=RAMD_DRIVER_USER — выбор зарегистрированного шаблона (как RESTORE).
;  Все регистры и флаги сохраняются; шаблон должен быть резидентным.
;
; 2022-02-02 SpaceEngineer
;----------------------------------------------------------------------------

setRAMDDriver:
            push  hl
            push  de
            push  bc
            push  af
            ld    a, b
            cp    RAMD_DRIVER_REGISTER
            jp z, setRAMDRegister
            cp    RAMD_DRIVER_BLOCK
            jp z, setRAMDBuiltin
            dec   b
            jp z, setRAMDDriver_1

; Операнд хранит шаблон RAMDISK.COM; до регистрации — встроенный блочный.
setRAMDBlockTemplate:
            ld    hl, driverBlockStart
            jp    setRAMDTemplate
setRAMDRegister:
            ld    (setRAMDBlockTemplate+1), hl
            jp    setRAMDDone

setRAMDDriver_1:
            ; Адреса побайтового драйвера
            ld    hl, driverByteStart
            jp    setRAMDTemplate
setRAMDBuiltin:
            ld    hl, driverBlockStart
setRAMDTemplate:
            push  hl
            ld    de, 32
            add   hl, de
            ex    de, hl
            pop   hl

            ; Копируем драйвер на адрес bc = bios_RAMDRead = 0FFC0h
setRAMDDriver_2:
            ld    bc, bios_RAMDRead
            call  memcpy_bc_hl

            ; Восстанавливаем регистры и выходим
setRAMDDone:
            pop   af
            pop   bc
            pop   de
            pop   hl
            ret

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

; Встроенный блочный драйвер целиком помещается в общее окно.
; FFC0h/FFD0h: C=страница, DE=сектор<<8 (E=0), HL=буфер основной RAM.
; Передаёт ровно 256 байт. C сохраняется; AF/B/DE/HL разрушаются.
; При выходе снова включена основная RAM; стек не переключается.
driverBlockStart:
            ld      a,c
            ld      (IO_PAGE_RAMD),a
            ld      a,(de)
            ld      (IO_PAGE_RAM),a
            ld      (hl),a
            inc     hl
            inc     e
            jp nz,  bios_RAMDRead
            ret
            nop
driverBlockWrite:
            ld      b,(hl)
            ld      a,c
            ld      (IO_PAGE_RAMD),a
            ld      a,b
            ld      (de),a
            ld      (IO_PAGE_RAM),a
            inc     hl
            inc     e
            jp nz,  bios_RAMDWrite
            ret
driverBlockEnd:
    ASSERT driverBlockWrite-driverBlockStart == bios_RAMDWrite-bios_RAMDRead
    ASSERT driverBlockEnd-driverBlockStart == 32

    INCLUDE "ramdBlockSector.inc"

; ---------------------------------------------------------------------------

; Проверка, что драйверы влезаеют в 32 байта разделяемой памяти Специалиста-MX


    IF driverByteWrite-driverByteStart+bios_RAMDRead != bios_RAMDWrite
        ASSERT 0
        DISPLAY /l, "Error! Byte briver entry point FFD0 has been shifted: ", driverByteWrite-driverByteStart+bios_RAMDRead, " != 0xFFD0"
    ENDIF

    IF driverByteEnd-driverByteStart > 32
        ASSERT 0
        DISPLAY /d, "Error! Byte briver did not fit: ", driverByteEnd-driverByteStart, " > 32 bytes"
    ENDIF
