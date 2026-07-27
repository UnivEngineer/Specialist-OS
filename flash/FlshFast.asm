;----------------------------------------------------------------------------
; MXOS
; FLSHFAST.COM - конвейерный драйвер flash-диска на четырёх AT29C040A
;
; Функции драйвера (номер в регистре b):
;   b == 1 - записать сектор (256 байт, номер сектора в de, буфер в hl);
;   b == 2 - прочитать сектор (256 байт, номер сектора в de, буфер в hl);
;   b == 3 - получить размер диска в секторах (de) и DISK_INFO (hl);
;   b == 4 - дождаться завершения всех отложенных операций записи.
;
; В отличие от Flash.asm запись возвращает управление после загрузки страницы
; во внутренний буфер выбранной микросхемы. Перед следующим обращением драйвер
; ждёт только эту микросхему. Остальные AT29C040A в это время могут продолжать
; внутренний цикл erase/program.
;
; Логическая раскладка диска полностью совпадает с Flash.asm: каждая
; микросхема занимает непрерывные 512 КБ. Поэтому ускорение четырьмя чипами
; проявляется при чередовании обращений к разным 512-КБ областям. Для ускорения
; обычной последовательной записи нужен отдельный явно маркированный формат
; с чередованием логических секторов между микросхемами.
;----------------------------------------------------------------------------

    INCLUDE "../include/mxos.inc"

; Команды bit set/reset управляющего регистра КР580ВВ55
LATCH_0 = 0Ah   ; выход защёлки КР580ИР22 = 0
LATCH_1 = 0Bh   ; выход защёлки КР580ИР22 = 1
WRITE_0 = 0Ch   ; сигнал записи = 0
WRITE_1 = 0Dh   ; сигнал записи = 1

; Маски порта C
MASK_STANDBY = 0C0h ; защёлка = 0, запись = 1, чтение = 1
MASK_READ    = 040h ; защёлка = 0, запись = 1, чтение = 0

PPI_MODE_WRITE = 080h ; порты A, B и C - выходы
PPI_MODE_READ  = 090h ; порт A - вход, порты B и C - выходы

DRIVE = 7             ; H:
DISK_SECTORS = 4 * 512 * 4

CHIP_STATE_SIZE  = 4
CHIP_STATE_COUNT = 4

CHIP_STATE.busy       = 0
CHIP_STATE.pageSelect = 1
CHIP_STATE.addrHi     = 2
CHIP_STATE.data       = 3

;----------------------------------------------------------------------------

    ORG     0DE00h

    ; Сбрасываем состояние, оставшееся в памяти от предыдущего драйвера.
    xor     a
    ld      (v_ppiMode), a
    ld      hl, v_chipStates
    ld      b, CHIP_STATE_SIZE * CHIP_STATE_COUNT
InitState:
    ld      (hl), a
    inc     hl
    dec     b
    jp nz,   InitState

    ; Устанавливаем драйвер диска H:.
    ld      a, DRIVE
    ld      hl, Driver
    call    bios_installDriver
    ld      (v_diskInfo), hl
    ret

;----------------------------------------------------------------------------

Driver:
    ld      a, b
    cp      3
    jp z,   FuncSize

    push    hl
    push    de
    push    bc

    dec     a
    jp z,   FuncWrite
    dec     a
    jp z,   FuncRead
    dec     a
    dec     a
    jp z,   FuncFlush

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
; Функция 3 - размер накопителя
;
; Выход:
;   de = число секторов;
;   hl = адрес DISK_INFO.

FuncSize:
    ld      hl, (v_diskInfo)
    ld      de, DISK_INFO.isValid
    add     hl, de
    ld      a, (hl)

    ld      de, DISK_SECTORS
    cp      DISK_VALID
    jp nz,  fsExit

    ld      hl, (v_diskInfo)
    ld      de, DISK_INFO.totalSectors
    add     hl, de
    ld      e, (hl)
    inc     hl
    ld      d, (hl)

fsExit:
    ld      hl, (v_diskInfo)
    ret

;----------------------------------------------------------------------------
; Функция 2 - чтение сектора
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
    call    SetReadMode

    ; D[4:0] задаёт A20:A16 flash-диска.
    ld      a, d
    and     01Fh
    or      MASK_READ
    ld      (IO_PROG_C), a

    ; E задаёт A15:A8, счётчик E - A7:A0.
    ld      d, e
    ld      e, 0
    call    LatchHiAddr

; Горячий цикл: 13 байт, 58 тактов/байт, 7424 мкс на сектор при 2 МГц.
; На байт: 1 запись и 1 чтение КР580ВВ55, 1 запись в ОЗУ.
ReadLoop:
    ld      a, e
    ld      (IO_PROG_B), a
    ld      a, (IO_PROG_A)
    ld      (hl), a
    inc     hl
    inc     e
    jp nz,  ReadLoop

    jp      Exit

;----------------------------------------------------------------------------
; Функция 1 - запись сектора
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
    call    SetWriteMode

    ; C4:C3 выбирают одну из четырёх микросхем.
    ld      c, WRITE_1
    ld      a, d
    and     018h
    ld      b, a
    call    DisableWriteProtection

    ; После последнего байта команды SDP нельзя превышать tBLC = 150 мкс.
    ; До первого импульса данных: 223 такта = 111,5 мкс при 2 МГц.
    ld      a, (v_pageSelect)
    or      MASK_STANDBY
    ld      (IO_PROG_C), a

    ; B и C содержат быстрые команды импульса /WE для цикла загрузки.
    ld      b, WRITE_0

    ld      d, e
    ld      e, 0
    call    LatchHiAddr

; Горячий цикл: 21 байт, 89 тактов/байт, 11392 мкс на сектор при 2 МГц.
; Обычный и худший путь совпадают. На байт: 4 записи КР580ВВ55 и 1 чтение ОЗУ.
WriteLoop:
    ld      a, e
    ld      (IO_PROG_B), a
    ld      a, (hl)
    ld      (IO_PROG_A), a
    ld      a, b
    ld      (IO_PROG_MODE), a
    ld      a, c
    ld      (IO_PROG_MODE), a
    inc     hl
    inc     e
    jp nz,  WriteLoop

    ; Сохраняем адрес последнего байта и ожидаемое значение DQ7.
    dec     hl
    ld      c, (hl)
    dec     e                       ; E = 0FFh

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

    jp      Exit

;----------------------------------------------------------------------------
; Функция 4 - барьер записи.
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
    push    af
    call    WaitChip
    pop     af
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
; Тайм-аут превышает максимальный tWC = 10 мс. ABI дискового драйвера не
; предусматривает возврат ошибки; после тайм-аута поведение совпадает с
; историческим Flash.asm - управление возвращается вызывающей программе.

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

    call    SetReadMode
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
    rrca
    rrca
    rrca
    add     a, a
    add     a, a
    ld      l, a
    ld      h, 0
    push    de
    ld      de, v_chipStates
    add     hl, de
    pop     de
    ret

;----------------------------------------------------------------------------

SetWriteMode:
    ld      a, (v_ppiMode)
    cp      PPI_MODE_WRITE
    ret z
    ld      a, PPI_MODE_WRITE
    ld      (IO_PROG_MODE), a
    ld      (v_ppiMode), a
    ret

SetReadMode:
    ld      a, (v_ppiMode)
    cp      PPI_MODE_READ
    ret z
    ld      a, PPI_MODE_READ
    ld      (IO_PROG_MODE), a
    ld      (v_ppiMode), a
    ret

;----------------------------------------------------------------------------
; Команда Software Data Protection перед загрузкой страницы.
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
; Защёлкивание A15:A8.
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
; Запись одного командного байта в выбранную микросхему.
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

v_diskInfo:   DW 0
v_statePtr:   DW 0
v_ppiMode:    DB 0
v_pageSelect: DB 0
v_waitSelect: DB 0

v_chipStates:
    BLOCK CHIP_STATE_SIZE * CHIP_STATE_COUNT, 0

;----------------------------------------------------------------------------

    ASSERT_DONT_FIT 0E000h

    END
