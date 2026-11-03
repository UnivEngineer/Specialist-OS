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
;  B=0 — обычный режим (без RAMD.COM — побайтовый), B=1 — сервисный.
;  B=2, HL=адрес 32-байтного шаблона — регистрация обычного режима.
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
            cp    2
            jp z, setRAMDRegister
            dec   b
            jp z, setRAMDDriver_1

; Операнд хранит шаблон, зарегистрированный RAMD.COM. Без него — байтовый.
setRAMDBlockTemplate:
            ld    hl, driverByteStart
            jp    setRAMDTemplate
setRAMDRegister:
            ld    (setRAMDBlockTemplate+1), hl
            jp    setRAMDDone

setRAMDDriver_1:
            ; Адреса побайтового драйвера
            ld    hl, driverByteStart
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

    INCLUDE "ramdByteSector.inc"

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
