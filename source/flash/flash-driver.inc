; Общее тело внешнего драйвера ВВ55: main-резидент и банковая DOS.
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

    IFNDEF DRIVER_IO_ONLY
FuncSize:
    ld      de, 0                 ; физическая ёмкость, патчит установщик
    ld      hl, 0                 ; DISK_INFO, патчит установщик
    ld      bc, 0
    or      a
    ret

    ENDIF

;----------------------------------------------------------------------------
; Чтение сектора.

FuncRead:
    ld      a, PPI_MODE_READ
    ld      (IO_PROG_MODE), a

    ld      a, d
    and     01Fh
    or      MASK_READ
    ld      (IO_PROG_C), a

    ld      d, e
    ld      e, 0
    call    LatchHiAddr

; Цикл 13 байт, 58 тактов/байт; чтение ВВ55 и запись в ОЗУ.
ReadLoop:
    ld      a, e
    ld      (IO_PROG_B), a
    ld      a, (IO_PROG_A)
    ld      (hl), a
    inc     hl
    inc     e
    jp nz,  ReadLoop

    or      a
    jp      Exit

;----------------------------------------------------------------------------
; Запись сектора.

FuncWrite:
    ld      a, PPI_MODE_WRITE
    ld      (IO_PROG_MODE), a

    ; C4:C3 выбирают микросхему; SDP всегда посылается в её нулевую банку.
    ld      a, d
    and     018h
    ld      b, a
    call    DisableWriteProtection

    ; D[4:0] задаёт A20:A16, E задаёт A15:A8.
    ld      a, d
    and     01Fh
    ld      (v_pageSelect), a
    or      MASK_STANDBY
    ld      (IO_PROG_C), a

    ld      b, d
    ld      d, e
    ld      e, 0
    call    LatchHiAddr

; Цикл 23 байта, 98 тактов/байт = 49 мкс при 2 МГц, меньше tBLC=150 мкс.
WriteLoop:
    ld      a, e
    ld      (IO_PROG_B), a
    ld      a, (hl)
    ld      (IO_PROG_A), a
    ld      a, WRITE_0
    ld      (IO_PROG_MODE), a
    ld      a, WRITE_1
    ld      (IO_PROG_MODE), a
    inc     hl
    inc     e
    jp nz,  WriteLoop

    ; Ожидаем DQ7 последнего байта. 768 чтений дают запас свыше tWC=10 мс.
    dec     hl
    ld      c, (hl)
    dec     e

    ld      a, PPI_MODE_READ
    ld      (IO_PROG_MODE), a
    ld      b, 3
    ld      l, 0
PollLoop:
    push    bc
    ld      a, (v_pageSelect)
    ld      b, a
    call    ReadByteFromChip
    pop     bc
    xor     c
    and     080h
    jp z,   PollDone
    dec     l
    jp nz,  PollLoop
    dec     b
    jp nz,  PollLoop

    jp      WriteFailed

PollDone:
    or      a
    jp      Exit


WriteFailed:
    ld      a, 1
    IFNDEF FLASH_BANKED_DRIVER
    ld      (bios_vars.diskWriteErr), a
    ENDIF
    scf
    jp      Exit

;----------------------------------------------------------------------------

DisableWriteProtection:
    push    de
    ld      a, 0AAh
    ld      de, 05555h
    call    WriteByteToChip
    ld      a, 055h
    ld      de, 02AAAh
    call    WriteByteToChip
    ld      a, 0A0h
    ld      de, 05555h
    call    WriteByteToChip
    pop     de
    ret

LatchHiAddr:
    ld      a, LATCH_1
    ld      (IO_PROG_MODE), a
    ld      a, d
    ld      (IO_PROG_B), a
    ld      a, LATCH_0
    ld      (IO_PROG_MODE), a
    ret

ReadByteFromChip:
    call    LatchHiAddr
    ld      a, e
    ld      (IO_PROG_B), a
    ld      a, b
    or      MASK_READ
    ld      (IO_PROG_C), a
    ld      a, (IO_PROG_A)
    ret

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
    ld      bc, DISK_TECH_FLASH * 256 + DISK_CAP_READ + DISK_CAP_WRITE + DISK_CAP_VERIFY
    ld      hl, DriverInfo
    or      a
    ret
DriverInfo:        DB "FLASH",0
DriverDescription: DB "0x512K sequential",0

; B=5: общий синхронный цикл B=1, B=2 и сравнения в DOS.
; BC/DE/HL сохраняются; CF=1 и diskWriteErr=1 при отказе.
FuncWriteChecked:
    IFDEF FLASH_BANKED_DRIVER
    ; Проверку после записи выполняет DOS, владеющая обоими буферами.
    ld      a,DOS_ERR_UNSUPPORTED
    scf
    ret
    ELSE
    push    bc
    ld      bc, Driver
    call    bios_diskWriteChecked
    pop     bc
    ret
    ENDIF

    ENDIF

v_pageSelect:   DB 0

