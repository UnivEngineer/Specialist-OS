; RAMD.COM — устанавливаемый блочный драйвер RAM-диска B:.
; Загрузка/запуск с 0000h; резидент F540h–F5FFh, хвосты FF00h.
; Вход установщика: стек вне нижних 32 КБ, прерывания запрещены MXOS.
; Выход CF=0 — установлен, CF=1 — отказ flush или нет RAM-диска.
; AF/BC/DE/HL разрушаются. AUTOEXEC не изменяется.
    INCLUDE "../include/mxos.inc"
    ORG 0
    jp Install

resident_file:
    DISP RAMD_RESIDENT_ADDR
; B=1/2: DE=сектор, HL=буфер, регистры сохраняются, CF=результат.
; B=3: DE=ёмкость в секторах, HL=DISK_INFO, BC=0, CF=0.
; B=FFh: HL=две строки, B=технология, C=возможности, CF=0.
Driver:
    push af
    ld a,b
    cp 0FFh
    jp z,Info
    cp 3
    jp z,Size
    push hl
    push bc
    push de
    call Validate
    jp c,Error
    dec b
    jp z,Write
    dec b
    jp nz,Error
    ld c,d
    ld d,e
    ld e,0
    call bios_RAMDRead
    jp Done
Write:
    ld c,d
    ld d,e
    ld e,0
    call bios_RAMDWrite
Done:
    pop de
    pop bc
    pop hl
    pop af
    or a
    ret
Error:
    ; Признак ошибки записи не меняем при отказе чтения/неизвестной функции.
    ld a,b
    dec a
    jp nz,ErrorPop
    ld a,1
    ld (bios_vars.diskWriteErr),a
ErrorPop:
    pop de
    pop bc
    pop hl
    pop af
    scf
    ret
Validate:
    ld a,(bios_vars.ramPageCount)
    cp d
    jp z,Invalid
    ret c
    ld a,e
    cp 0FFh
    jp z,Invalid
    ld a,h
    cp 0FEh
    jp c,Valid
    jp nz,Invalid
    ld a,l
    or a
    ret z
Invalid:
    scf
    ret
Valid:
    or a
    ret
Size:
    pop af
    ld a,(bios_vars.ramPageCount)
    ld d,a
    ld e,0
    ld bc,0
InfoPtr:
    ld hl,0                     ; адрес DISK_INFO задаёт установщик
    or a
    ret
Info:
    pop af
    ld bc,DISK_TECH_RAM*256+DISK_CAP_READ+DISK_CAP_WRITE
    ld hl,DriverInfo
    or a
    ret
DriverInfo: DB "RAMD",0,"R4 banked",0
resident_end:
    ASSERT resident_end <= RAMD_RESIDENT_ADDR + 0C0h
    ENT
resident_file_end:

    INCLUDE "r4.inc"

Install:
    call bios_flushVolume
    ret c
    ld a,(bios_vars.ramPageCount)
    or a
    jp z,InstallFailed
    ld b,1
    call bios_setRAMDDriver
    ld hl,resident_file
    ld de,resident_file_end
    ld bc,RAMD_RESIDENT_ADDR
    call bios_memcpy_bc_hl
    ld hl,ramdTailImage
    ld de,ramdTailImageEnd
    ld bc,0FF00h
    call bios_memcpy_bc_hl
    ld a,(bios_vars.ramPageCount)
    ld b,a
    xor a
InstallPage:
    ld de,ramdTailImage
    ld hl,0FF00h
InstallByte:
    push bc
    push af
    ld a,(de)
    ld c,a
    pop af
    call bios_RAMDWrite
    pop bc
    inc de
    inc l
    ld c,a
    ld a,l
    cp ramdBankTailEnd-regReadPop
    ld a,c
    jp nz,InstallByte
    inc a
    dec b
    jp nz,InstallPage
    ; Сначала готовы все резиденты, затем регистрируем драйвер B:.
    ld a,1
    ld hl,Driver
    call bios_installDriver
    ld (InfoPtr+1),hl
    ld hl,driverBufferedStart
    ld b,2
    call bios_setRAMDDriver
    ld b,0
    call bios_setRAMDDriver
    xor a
    ret
InstallFailed:
    ld hl,txtNoRAM
    call bios_printString
    scf
    ret
txtNoRAM: DB "RAMD: no RAM disk",00Ah,0
    END
