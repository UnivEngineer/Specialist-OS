;----------------------------------------------------------------------------
; MXOS
; FLASH.COM - драйвер флеш-диска на 1–4 AT29C040A с определением числа чипов.
; COM загружается с 0000h; после ID резидент копируется в F600h–F7FFh.
;
; Номер функции передаётся в регистре b:
;   b == 1 - запись сектора, de=номер, hl=буфер;
;   b == 2 - чтение сектора, de=номер, hl=буфер;
;   b == 3 - размер в de, адрес DISK_INFO в hl, bc=0 (линейное выделение);
;   b == 5 - записать сектор, затем прочитать и сравнить все 256 байт;
;   b == 0FFh - адрес имени и описания в hl (две строки с нулём).
;
; Для чтения и записи CF=0 означает успех, CF=1 - ошибку. После загрузки
; страницы ожидается завершение по DATA polling. Полное сравнение выполняется
; в функции B=5; обычная запись B=1 не выполняет полного сравнения.
;
; Порт A - данные, B - младший адрес и вход защёлки старшего адреса ИР22.
; C0-C2 - A16-A18; C3-C4 - чип; C5 - защёлка, C6 - /WR, C7 - /RD.
;
; 2022-01-25 Разработано SpaceEngineer
; 2026-10-31 Объединены Flash и FlashChk; B=5 выбирает запись с проверкой.
;----------------------------------------------------------------------------

    INCLUDE "../include/mxos.inc"

LATCH_0 = 00Ah
LATCH_1 = 00Bh
WRITE_0 = 00Ch
WRITE_1 = 00Dh

MASK_STANDBY = 0C0h
MASK_READ    = 040h

PPI_MODE_WRITE = 080h
PPI_MODE_READ  = 090h

DRIVE        = 7

;----------------------------------------------------------------------------

RESIDENT_ADDRESS = DISK_DRIVER_ADDR
SYSTEM_FLASH = 0

    ORG 0
    jp Install
resident_file:
    DISP RESIDENT_ADDRESS

Driver:
    ld      a, b
    cp      0FFh
    jp z,   FuncInfo
    cp      5
    jp z,   FuncWriteChecked
    cp      3
    jp z,   FuncSize

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

FuncSize:
    ld      de, 0                 ; физическая ёмкость, патчит установщик
    ld      hl, 0                 ; DISK_INFO, патчит установщик
    ld      bc, 0
    or      a
    ret

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
    ld      (bios_vars.diskWriteErr), a
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
    push    bc
    ld      bc, Driver
    call    bios_diskWriteChecked
    pop     bc
    ret

v_pageSelect:   DB 0

resident_end:
    ASSERT_DONT_FIT DISK_DRIVER_ADDR + 0200h
    ENT
resident_file_end:
    INCLUDE "flash-install.inc"


    END
