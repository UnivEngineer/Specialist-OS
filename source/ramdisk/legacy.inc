; Переключатель исторической реализации: 1 — CP/M buf16, 0 — прежний R4.
    IFNDEF RAMD_USE_CPM
RAMD_USE_CPM = 1
    ENDIF
    ASSERT RAMD_USE_CPM == 0 || RAMD_USE_CPM == 1
    ORG 0
    jp BankInstall

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
    IF RAMD_USE_CPM
    call cpmInitRead
    ELSE
    call regInitRead
    ENDIF
    jp Done
Write:
    ld c,d
    ld d,e
    ld e,0
    IF RAMD_USE_CPM
    call cpmInitWrite
    ELSE
    call regInitWrite
    ENDIF
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
    ld (bios_upper.diskWriteErr),a
ErrorPop:
    pop de
    pop bc
    pop hl
    pop af
    scf
    ret
Validate:
    ld a,(bios_upper.ramPageCount)
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
    ld a,(bios_upper.ramPageCount)
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
    IF RAMD_USE_CPM
    INCLUDE "cpm-entry.inc"
DriverInfo: DB "RAMD",0,"CP/M16",0
    ELSE
DriverInfo: DB "RAMD",0,"R4 BANKED",0
    ENDIF
resident_end:
    ASSERT resident_end <= RAMD_RESIDENT_ADDR + 0C0h
    ENT
resident_file_end:

    IF RAMD_USE_CPM
    INCLUDE "cpm.inc"
    ELSE
    INCLUDE "r4.inc"
    ENDIF

BankInstall:
    INCLUDE "bank-install.inc"
Install:
    ld a,(bios_upper.koi7)
    push af
    xor a
    call bios_setFontEncoding
    call InstallBody
    pop bc
    push af
    ld a,b
    call bios_setFontEncoding
    pop af
    ret
InstallBody:
    MXOS_CALL MXOS_DOS_FLUSH_VOLUME
    ret c
    ld a,(bios_upper.ramPageCount)
    or a
    jp z,InstallFailed
    ld b,RAMD_DRIVER_BYTE
    call bios_setRAMDDriver
    ld hl,resident_file
    ld de,resident_file_end
    ld bc,RAMD_RESIDENT_ADDR
    call bios_memcpy_bc_hl
    ld hl,ramdTailImage
    ld de,ramdTailImageEnd
    ld bc,0FF00h
    call bios_memcpy_bc_hl
    ld a,(bios_upper.ramPageCount)
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
    cp ramdBankTailEnd-0FF00h
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
    ld b,RAMD_DRIVER_REGISTER
    call bios_setRAMDDriver
    ld b,RAMD_DRIVER_RESTORE
    call bios_setRAMDDriver
    xor a
    ret
InstallFailed:
    ld hl,txtNoRAM
    call bios_printString
    scf
    ret
txtNoRAM: DB "RAMDISK: NO RAM DISK",00Ah,0
