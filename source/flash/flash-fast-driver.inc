; Банковая копия физического FlashFst; синхронная проверка локальна.
Driver:
    ld      a, b
    IFNDEF DRIVER_IO_ONLY
    cp      0FFh
    jp z,   FuncInfo
    cp      5
    jp z,   FuncWriteChecked
    cp      3
    jp z,   FuncSize

    ENDIF

    push    hl
    push    de
    push    bc

    cp      4
    jp z,   FuncFlush
    ld      a, d
SectorLimit:
    cp      0                     ; старший байт физической ёмкости
    jp nc,  InvalidFunction
    call    MapSector
    ld      a, b
    dec     a
    jp z,   FuncWrite
    dec     a
    jp z,   FuncRead
InvalidFunction:
    scf

Exit:
    push    af
    ld      a, MASK_STANDBY
    ld      (IO_PROG_C), a
    pop     af
    pop     bc
    pop     de
    pop     hl
    ret

;----------------------------------------------------------------------------
; Функция 3 - определение объёма накопителя
;
; Выход:
;   de = число секторов;
;   hl = адрес DISK_INFO.

    IFNDEF DRIVER_IO_ONLY
FuncSize:
    ld      de, 0                 ; физическая ёмкость, патчит установщик
    ld      hl, 0                 ; DISK_INFO, патчит установщик
    ld      bc, 0800h
    or      a
    ret

    ENDIF

;----------------------------------------------------------------------------
; Функция 2 - чтение сектора из флеш-памяти
;
; Вход:
;   de = номер сектора;
;   hl = адрес буфера.

FuncRead:
    ; Чтение допустимо сразу, если программируется другая микросхема.
    push    hl
    ld      a, d
    call    GetChipState
    call    WaitChip
    pop     hl
    jp c,   Exit
    ld      a, PPI_MODE_READ
    ld      (IO_PROG_MODE), a

    ; D[4:0] задаёт A20:A16 flash-диска.
    ld      a, d
    and     01Fh
    or      MASK_READ
    ld      (IO_PROG_C), a

    ; E задаёт A15:A8; младшие адреса затем перебирает C.
    ld      d, e
    call    LatchHiAddr
    ex      de, hl              ; DE=назначение, HL больше не адрес буфера
    ld      hl, IO_PROG_A
    ld      c, 0

; 11 байт, 52 такта/байт: HL=порт данных, DE=буфер, C=младший адрес.
ReadLoop:
    ld      a, c
    ld      (IO_PROG_B), a
    ld      a, (hl)
    ld      (de), a
    inc     de
    inc     c
    jp nz,  ReadLoop

    or      a
    jp      Exit

;----------------------------------------------------------------------------
; Функция 1 - запись сектора во флеш-память
;
; Вход:
;   de = номер сектора;
;   hl = адрес буфера.
;
; Выход:
;   страница принята выбранной AT29C040A; внутренний цикл erase/program может
;   ещё продолжаться. Любое следующее обращение к этому чипу синхронизируется
;   в WaitChip.

FuncWrite:
    ; Запоминаем полный выбор страницы и находим состояние выбранного чипа.
    ld      a, d
    and     01Fh
    ld      (v_pageSelect), a

    push    hl
    ld      a, d
    call    GetChipState
    ld      (v_statePtr), hl

    ; Нельзя загружать новую страницу, пока этот же чип программирует старую.
    call    WaitChip
    pop     hl
    jp c,   Exit
    ld      a, PPI_MODE_WRITE
    ld      (IO_PROG_MODE), a

    ; C4:C3 выбирают одну из четырёх микросхем.
    ld      a, d
    and     018h
    ld      b, a
    call    DisableWriteProtection

    ; После последнего байта команды SDP нельзя превышать tBLC = 150 мкс.
    ; Настройка порта и регистров до первого байта укладывается в tBLC.
    ld      a, (v_pageSelect)
    or      MASK_STANDBY
    ld      (IO_PROG_C), a

    ld      d, e
    ld      e, 0
    call    LatchHiAddr

    ld      c, d                ; старший адрес для DATA polling
    ex      de, hl              ; DE=источник
    ld      hl, IO_PROG_MODE
    ld      b, 0

; 17 байт, 78 тактов/байт = 39 мкс при 2 МГц, меньше tBLC=150 мкс.
; HL=управление ВВ55, DE=источник, B=младший адрес, C=старший адрес.
WriteLoop:
    ld      a, b
    ld      (IO_PROG_B), a
    ld      a, (de)
    ld      (IO_PROG_A), a
    ld      (hl), WRITE_0
    ld      (hl), WRITE_1
    inc     de
    inc     b
    jp nz,  WriteLoop

    ex      de, hl              ; HL=конец источника
    ld      d, c
    ld      e, 0FFh             ; последний адрес выбранной страницы
    dec     hl
    ld      c, (hl)              ; ожидаемый DQ7

    ld      hl, (v_statePtr)
    inc     hl
    ld      a, (v_pageSelect)
    ld      (hl), a                 ; pageSelect
    inc     hl
    ld      (hl), d                 ; addrHi
    inc     hl
    ld      (hl), c                 ; data

    ld      hl, (v_statePtr)
    ld      (hl), 1                 ; busy, записывается последним
    or      a                       ; CF=0: страница принята

    jp      Exit

;----------------------------------------------------------------------------
; Функция 4 - ожидание окончания записи во все микросхемы.
;
; Дожидается всех четырёх микросхем. Это расширение ABI предназначено для
; тестов, безопасного выключения и программ, которым нужен явный flush.

FuncFlush:
    call    WaitAll
    jp      Exit

;----------------------------------------------------------------------------
; Ожидание всех незавершённых операций.
;
; Разрушает:
;   af, de, hl.

WaitAll:
    ld      hl, v_chipStates
    ld      a, CHIP_STATE_COUNT
waLoop:
    push    bc
    ld      b, a
    call    WaitChip
    ld      a, b
    pop     bc
    ret c
    ld      de, CHIP_STATE_SIZE
    add     hl, de
    dec     a
    jp nz,  waLoop
    ret

;----------------------------------------------------------------------------
; Ожидание выбранной микросхемы по DATA polling.
;
; Вход:
;   hl = адрес CHIP_STATE.
;
; Выход:
;   busy = 0.
;
; Сохраняет:
;   bc, de, hl.
;
; Разрушает:
;   af.
;
; Тайм-аут превышает tWC=10 мс; при отказе CF=1 и diskWriteErr=1.

WaitChip:
    ld      a, (hl)
    or      a
    ret z

    push    hl
    push    de
    push    bc

    inc     hl
    ld      a, (hl)
    ld      (v_waitSelect), a
    inc     hl
    ld      d, (hl)
    inc     hl
    ld      c, (hl)

    ld      a, PPI_MODE_READ
    ld      (IO_PROG_MODE), a
    call    LatchHiAddr

    ld      a, 0FFh
    ld      (IO_PROG_B), a
    ld      a, (v_waitSelect)
    or      MASK_READ
    ld      (IO_PROG_C), a

    ; Горячий цикл: готовность за 34 такта; занятость - 49 тактов на проверку.
    ; Худший путь (512 чтений КР580ВВ55) занимает около 12,6 мс при 2 МГц.
    ld      b, 2
    ld      l, 0
wcPoll:
    ld      a, (IO_PROG_A)
    xor     c
    and     080h
    jp z,   wcDone
    dec     l
    jp nz,  wcPoll
    dec     b
    jp nz,  wcPoll

    pop     bc
    pop     de
    pop     hl
    xor     a
    ld      (hl), a
    inc     a
    IFNDEF DRIVER_IO_ONLY
    ld      (bios_upper.diskWriteErr), a
    ENDIF
    scf
    ret

wcDone:
    pop     bc
    pop     de
    pop     hl
    xor     a
    ld      (hl), a
    ret

;----------------------------------------------------------------------------
; Получение адреса состояния микросхемы.
;
; Вход:
;   a = старший байт логического номера сектора.
;
; Выход:
;   hl = адрес CHIP_STATE.
;
; Сохраняет:
;   de.
;
; Разрушает:
;   af.

GetChipState:
    and     018h
    rrca                       ; 0/8/16/24 -> 0/4/8/12
    ld      l, a
    ld      h, 0
    push    de
    ld      de, v_chipStates
    add     hl, de
    pop     de
    ret

;----------------------------------------------------------------------------

; Отключение программной защиты от записи перед загрузкой страницы.
;
; Вход:
;   b = C4:C3 выбора микросхемы (0, 8, 16 или 24).
;
; Сохраняет:
;   de.
;
; Разрушает:
;   af.

DisableWriteProtection:
    push    de
    ld      a, 0AAh
    ld      de, 5555h
    call    WriteByteToChip
    ld      a, 055h
    ld      de, 2AAAh
    call    WriteByteToChip
    ld      a, 0A0h
    ld      de, 5555h
    call    WriteByteToChip
    pop     de
    ret

;----------------------------------------------------------------------------
; Защёлкивание старшего байта адреса A15:A8 в ИР22.
;
; Вход:
;   d = A15:A8.
;
; Сохраняет:
;   bc, de, hl.
;
; Разрушает:
;   af.

LatchHiAddr:
    ld      a, LATCH_1
    ld      (IO_PROG_MODE), a
    ld      a, d
    ld      (IO_PROG_B), a
    ld      a, LATCH_0
    ld      (IO_PROG_MODE), a
    ret

;----------------------------------------------------------------------------
; Запись одного командного байта в выбранный чип флеш-памяти.
;
; Вход:
;   de = адрес внутри микросхемы;
;   b  = C4:C3 выбора микросхемы;
;   a  = байт команды.
;
; Разрушает:
;   af.

WriteByteToChip:
    ld      (IO_PROG_A), a
    call    LatchHiAddr
    ld      a, e
    ld      (IO_PROG_B), a
    ld      a, b
    or      MASK_STANDBY
    ld      (IO_PROG_C), a
    ld      a, WRITE_0
    ld      (IO_PROG_MODE), a
    ld      a, WRITE_1
    ld      (IO_PROG_MODE), a
    ret

;----------------------------------------------------------------------------


    INCLUDE "flash-map.inc"

    IFNDEF DRIVER_IO_ONLY
; B=0FFh: HL=две строки, B=технология, C=возможности (DISK_CAP_*).
FuncInfo:
    ld      bc, DISK_TECH_FLASH * 256 + DISK_CAP_READ + DISK_CAP_WRITE + DISK_CAP_VERIFY + DISK_CAP_FLUSH
    ld      hl, DriverInfo
    or      a
    ret
DriverInfo:        DB "AT2MXF",0
DriverDescription: DB "0X512K PIPELINE",0

; B=5: общий синхронный цикл B=1, B=2 и сравнения в DOS.
; BC/DE/HL сохраняются; CF=1 и diskWriteErr=1 при отказе.
FuncWriteChecked:
    IFDEF FLASH_BANKED_DRIVER
    ; Общая проверка записи выполняется DOS, которая владеет буфером чтения.
    ld      a,DOS_ERR_UNSUPPORTED
    scf
    ret
    ELSE
    push    bc
    ld      bc, Driver
    call    diskWriteChecked
    pop     bc
    ret
    ENDIF

    ENDIF

v_statePtr:     DW 0
v_pageSelect:   DB 0
v_waitSelect:   DB 0

v_chipStates:
    BLOCK CHIP_STATE_SIZE * CHIP_STATE_COUNT, 0

;----------------------------------------------------------------------------

