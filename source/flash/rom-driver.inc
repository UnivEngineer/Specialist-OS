; Физический драйвер внешнего ROM, исполняется в странице DOS.
Driver:
    ; B=1/2 сохраняют BC/DE/HL, AF и флаги разрушаются.
    ; Для B=3 и B=0FFh регистровый контракт приведён у обработчиков.
    ld      a, b
    IFNDEF DRIVER_IO_ONLY
    cp      0FFh
    jp z,   FuncInfo
    ENDIF
    cp      1
    jp nz,  DriverNotWrite
    ld      (bios_upper.diskWriteErr), a
    scf
    ret                 ; запись - не поддерживается
DriverNotWrite:
    cp      2
    jp z,   FuncRead    ; чтение
    IFNDEF DRIVER_IO_ONLY
    cp      3
    jp z,   FuncSize    ; размер
    ENDIF
    scf                 ; неизвестная функция, включая запись B=5
    ret                 ; иначе выход

;----------------------------------------------------------------------------
; Функция 3 - определение объёма накопителя.
;
; Выход:
;   de = ёмкость в 256-байтных блоках (при установке берётся из BPB);
;   hl = адрес DISK_INFO;
;   bc = 0, подсказка линейного выделения; CF=0.
; Геометрия выбранного тома не меняет уже установленную ёмкость.
    IFNDEF DRIVER_IO_ONLY
FuncSize:
    ld      de, DISK_SECTORS
    ld      bc, 0
    ld      hl, (v_diskInfo)
    or      a
    ret

    ENDIF
;----------------------------------------------------------------------------
; Функция 2 - чтение сектора из внешнего ROM.
;
; Вход:
;   de = номер 256-байтного блока; границы тома проверяет физический шлюз DOS.
;   hl = адрес буфера в основном ОЗУ.
;
; Выход:
;   буфер заполнен; при вызове через Driver CF=0.
;
; Сохраняет:
;   bc, de, hl.
;
; Разрушает:
;   af и флаги, кроме CF (сохраняется из диспетчера).

FuncRead:
RomReadSelect:
    jp      FuncReadLegacy      ; установщик выбирает интерфейс по BPB
FuncReadLegacy:
    ld a,d
    or a
    jp z,FuncReadReady
    ld a,DOS_ERR_ARGUMENT
    scf
    ret
FuncReadReady:
    ; Сохранение регистров
    push    hl
    push    de
    push    bc

    ; Настройка портов ВВ55
    ld      a, 90h              ; порт A - ввод, порты B и C - вывод 
    ld      (IO_PROG_MODE), a
    ld      a, 0Dh              ; вывод С6 порта клавиатуры = 1
    ld      (IO_KEYB_MODE), a

    ; В 64-КБ ROM адрес сектора равен E << 8; исходный D не используется.
    ld      d, e
    ld      e, 0

    ; Старший адрес неизменен в пределах 256-байтного сектора.
    ld      a, d
    ld      (IO_PROG_C), a
    ex      de, hl
    ld      hl, IO_PROG_A
    ld      c, 0

; 11 байт, 52 такта/байт; одна запись адреса, одно чтение ВВ55.
ReadLoop:
    ld      a, c
    ld      (IO_PROG_B), a
    ld      a, (hl)
    ld      (de), a
    inc     de
    inc     c
    jp nz,  ReadLoop

    ; Восстановление портов, регистров и выход
    push    af
    ld      a, 0Ch          ; вывод С6 порта клавиатуры = 0
    ld      (IO_KEYB_MODE), a
    ld      a, 9Bh          ; Порты A, B, C - ввод
    ld      (IO_PROG_MODE), a
    pop     af
    pop     bc
    pop     de
    pop     hl
    ret

; Нынешняя плата flash-диска: C4:C3=чип, C2:C0=A18:A16,
; A15:A8 удерживаются внешней защёлкой. Команды flash не посылаются.
; BC/DE/HL сохраняются; AF разрушены, CF=0 при успехе.
; Цикл: 11 байт, 52 такта/байт, 256 чтений PPI и записей ОЗУ.
FlashRead:
    ld      a,d
    cp      020h               ; четыре 512-КиБ гнезда, максимум 2 МиБ
    jp nc,  RomReadBad
    push    hl
    push    de
    push    bc
    ld      a,090h
    ld      (IO_PROG_MODE),a
    ld      a,d
    or      040h               ; /OE активен, /WE неактивен
    ld      (IO_PROG_C),a
    ld      a,00Bh             ; установить PC5 защёлки адреса
    ld      (IO_PROG_MODE),a
    ld      a,e
    ld      (IO_PROG_B),a
    ld      a,00Ah
    ld      (IO_PROG_MODE),a
    ex      de,hl
    ld      hl,IO_PROG_A
    ld      c,0
FlashReadLoop:
    ld      a,c
    ld      (IO_PROG_B),a
    ld      a,(hl)
    ld      (de),a
    inc     de
    inc     c
    jp nz,  FlashReadLoop
    ld      a,0C0h             ; оба управляющих сигнала неактивны
    ld      (IO_PROG_C),a
    pop     bc
    pop     de
    pop     hl
    or      a
    ret
RomReadBad:
    ld      a,DOS_ERR_ARGUMENT
    scf
    ret

;----------------------------------------------------------------------------
; Чтение байта ROM; вспомогательная точка, секторный цикл её не вызывает.
;
; Вход:
;   de = адрес в 64-КБ ROM;
;   ВВ55 уже настроен на чтение, PC6 клавиатурного ВВ55 установлен.
;
; Выход:
;   a = байт ROM; разрешение ROM и направления портов не меняются.
;
; Сохраняет:
;   bc, de, hl и флаги.

ReadByteFromChip:
    ex      de, hl
    ld      (IO_PROG_B), hl
    ld      a, (IO_PROG_A)
    ex      de, hl
    ret

;----------------------------------------------------------------------------
; Функция 0FFh - дескриптор физического драйвера для инфо-панели NC.
;
; Выход:
;   hl = адрес имени и описания (две ASCII-строки с нулём), CF=0;
;   b = DISK_TECH_ROM, c = DISK_CAP_READ.
;
; Сохраняет:
;   de.
;
; Разрушает:
;   af и флаги.
    IFNDEF DRIVER_IO_ONLY
FuncInfo:
    ld      bc, DISK_TECH_ROM * 256 + DISK_CAP_READ
    ld      hl, DriverInfo
    or      a
    ret
DriverInfo:     DB "ROM2M",0,"64K READ ONLY",0

;----------------------------------------------------------------------------
; Адрес DISK_INFO, полученный при регистрации драйвера.

v_diskInfo:     DW 0

;----------------------------------------------------------------------------

    ENDIF
