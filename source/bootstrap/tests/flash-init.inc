; Инициализация в DOS: обнаружение 1–4 чипов, без вызова основного BIOS.
; Выход CF/A=код DOS; AF/BC/DE/HL разрушены. Функции драйвера 1/2/3/FF.
flashDosInit:
    call ProbeDelay
    xor a
    ld (flashChipCount),a
    ld (SectorLimit+1),a
    ld (FuncSize+2),a
    ld b,a
    ld hl,v_chipMap
    ld (flashMapTarget),hl
flashProbeLoop:
    push bc
    call ParallelProbe
    ld a,b
    cp 0FFh
    jp z,flashProbeAbsent
    cp 01Fh
    jp nz,flashProbeFailed
    ld a,c
    cp 0A4h
    jp nz,flashProbeFailed
    pop bc
    ld hl,(flashMapTarget)
    ld (hl),b
    inc hl
    ld (flashMapTarget),hl
    ld hl,flashChipCount
    inc (hl)
    jp flashProbeNext
flashProbeAbsent:
    pop bc
flashProbeNext:
    ld a,b
    add a,8
    ld b,a
    cp 020h
    jp nz,flashProbeLoop
    ld a,(flashChipCount)
    or a
    jp z,flashProbeEmpty
    add a,'0'
    ld (DriverDescription),a
    ld a,(flashChipCount)
    add a,a
    add a,a
    add a,a
    ld (SectorLimit+1),a
    ld (FuncSize+2),a
    xor a
    ret
flashProbeFailed:
    pop bc
flashProbeEmpty:
    ld a,DOS_ERR_NO_DRIVER
    scf
    ret
ProbeDelay:
    ld hl,1280
flashProbeDelay:
    dec hl
    ld a,h
    or l
    jp nz,flashProbeDelay
    ret
flashChipCount: DB 0
flashMapTarget: DW 0
    INCLUDE "../../flash/flash-probe.inc"
