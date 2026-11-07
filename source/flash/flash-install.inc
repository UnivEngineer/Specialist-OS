; Нерезидентный установщик, COM загружается с 0000h. Резидент копируется
; только после успешного ID; при отказе прежний драйвер остаётся установлен.
; Временный ID использует 8C00h–8CFFh, общий системный резерв переходников.
; Выход CF=0: установлен; CF=1: ошибка flush, нет ID или неподдерживаемый чип.
Install:
    ld a,(bios_vars.koi7)
    push af
    xor a
    ld (bios_vars.koi7),a
    call InstallBody
    pop bc
    push af
    ld a,b
    ld (bios_vars.koi7),a
    pop af
    ret
InstallBody:
    MXCALL MX_DOS_FLUSHVOLUME
    ret c
    call ProbeDelay                  ; завершить ранее запущенное программирование
    IF SYSTEM_FLASH
    ld hl,sys_probe_file
    ld de,sys_probe_file_end
    ld bc,08C00h
    call bios_memcpy_bc_hl
    call SysProbe
    call DecodeSystemID
    jp c,InstallFailed
    ld (resident_file+FuncSize+1-Driver),hl
    ld (resident_file+SectorLimit+1-Driver),a
    ld a,c
    ld (resident_file+v_halfPage-Driver),a
    ex de,hl
    ld bc,resident_file+DriverDescription-Driver
    ld d,h
    ld e,l
    inc de
    inc de
    inc de
    call bios_memcpy_bc_hl
    ELSE
    ld b,0
    ld hl,resident_file+v_chipMap-Driver
    ld (mapTarget),hl
    xor a
    ld (chipCount),a
probeSocket:
    push bc
    call ParallelProbe
    ld a,b
    cp 0FFh                         ; пустое гнездо: шина данных отпущена
    jp z,probeAbsent
    cp 01Fh
    jp nz,probeUnsupported
    ld a,c
    cp 0A4h                         ; внешняя плата рассчитана на AT29C040A
    jp nz,probeUnsupported
    pop bc
    ld hl,(mapTarget)
    ld (hl),b
    inc hl
    ld (mapTarget),hl
    ld hl,chipCount
    inc (hl)
    jp probeNext
probeAbsent:
    pop bc
probeNext:
    ld a,b
    add a,8
    ld b,a
    cp 020h
    jp nz,probeSocket
    ld a,(chipCount)
    or a
    jp z,InstallFailed
    add a,'0'
    ld (resident_file+DriverDescription-Driver),a
    ld a,(chipCount)
    add a,a
    add a,a
    add a,a
    ld (resident_file+FuncSize+2-Driver),a
    ld (resident_file+SectorLimit+1-Driver),a
    ENDIF
    ld hl,resident_file
    ld de,resident_file_end
    ld bc,RESIDENT_ADDRESS
    call bios_memcpy_bc_hl
    ld a,DRIVE
    ld hl,Driver
    call bios_installDriver
    ld (FuncSize+4),hl
    IF SYSTEM_FLASH
    xor a
    MXCALL MX_DOS_DRIVERREFRESH
    ret c
    ENDIF
    xor a
    ret
    IF !SYSTEM_FLASH
probeUnsupported:
    pop bc
    ENDIF
InstallFailed:
    ld hl,txtIDError
    call bios_printString
    scf
    ret
txtIDError: DB "FLASH: unsupported / missing chip",00Ah,0

; 30720 тактов цикла (15,36 мс при 2 МГц, 12,288 мс при 2,5 МГц).
; HL/AF разрушены; BC/DE сохраняются. Нужна пауза 10 мс входа/выхода ID.
ProbeDelay:
    ld hl,1280
probeDelayLoop:
    dec hl
    ld a,h
    or l
    jp nz,probeDelayLoop
    ret

    IF !SYSTEM_FLASH
chipCount: DB 0
mapTarget: DW 0
    INCLUDE "flash-probe.inc"
    ELSE
; B=manufacturer/C=device. HL=секторы, A=старший байт ёмкости,
; C=конец первой полустраницы (80h для 128-КБ чипа, иначе 0).
DecodeSystemID:
    ld a,b
    cp 01Fh
    jp nz,decodeBad
    ld a,c
    ld hl,0200h
    ld de,txt128
    ld c,080h
    cp 0D5h
    jp z,decodeGood
    ld hl,0400h
    ld de,txt256
    ld c,0
    cp 0DAh
    jp z,decodeGood
    ld hl,0800h
    ld de,txt512
    cp 0A4h
    jp nz,decodeBad
decodeGood:
    ld a,h
    or a
    ret
decodeBad:
    scf
    ret
txt128: DB "128"
txt256: DB "256"
txt512: DB "512"

sys_probe_file:
    DISP 08C00h
SysProbe:
    xor a
    ld (IO_PAGE_ROM),a
    ld c,090h
    call SysCommand
    call SysDelay
    ld a,(0)
    ld b,a
    ld a,(1)
    ld c,a
    push bc
    ld c,0F0h
    call SysCommand
    call SysDelay
    pop bc
    ld (IO_PAGE_RAM),a
    ret
SysCommand:
    ld a,0AAh
    ld (05555h),a
    ld a,055h
    ld (02AAAh),a
    ld a,c
    ld (05555h),a
    ret
SysDelay:
    ld hl,1280
sysDelayLoop:
    dec hl
    ld a,h
    or l
    jp nz,sysDelayLoop
    ret
    ASSERT $ <= 08D00h
    ENT
sys_probe_file_end:
    ENDIF
