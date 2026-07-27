;----------------------------------------------------------------------------
; MXOS
; FLASH.COM - проверяемый драйвер внешнего flash-диска AT29C040A
;
; ABI:
;   b=1 - запись сектора, de=номер, hl=буфер;
;   b=2 - чтение сектора, de=номер, hl=буфер;
;   b=3 - размер в de, адрес DISK_INFO в hl.
;
; Для чтения и записи CF=0 означает успех, CF=1 - ошибку. После загрузки
; страницы запись ожидается по DATA polling, затем проверяются все 256 байт.
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
DISK_SECTORS = 4 * 512 * 4

;----------------------------------------------------------------------------

    ORG     0DE00h

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
    ; Оригинальные адреса нужны для полной проверки после программирования.
    push    hl
    push    de

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

; 22 байта, 100 тактов/байт = 50 мкс при 2 МГц, меньше tBLC=150 мкс.
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

    pop     de
    pop     hl
    jp      WriteFailed

PollDone:
    pop     de
    pop     hl
    call    VerifySector
    jp      Exit

;----------------------------------------------------------------------------
; Полное сравнение записанного сектора с исходным буфером.
;
; Вход: de=логический сектор, hl=буфер.

VerifySector:
    ld      a, PPI_MODE_READ
    ld      (IO_PROG_MODE), a

    ld      a, d
    and     01Fh
    or      MASK_READ
    ld      (IO_PROG_C), a

    ld      d, e
    ld      e, 0
    call    LatchHiAddr

VerifyLoop:
    ld      a, e
    ld      (IO_PROG_B), a
    ld      a, (IO_PROG_A)
    cp      (hl)
    jp nz,  WriteFailed
    inc     hl
    inc     e
    jp nz,  VerifyLoop

    or      a
    ret

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

v_diskInfo:   DW 0
v_pageSelect: DB 0

    ASSERT_DONT_FIT 0E000h

    END
