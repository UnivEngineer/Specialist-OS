; One-shot DOS launcher for the resident banked shell.
    INCLUDE "../include/mxos.inc"
    INCLUDE "bank-api.inc"
    IFNDEF DRIVER_ORIGIN
DRIVER_ORIGIN = DOS_BANK_DRIVER_ADDR
    ENDIF
    ORG DRIVER_ORIGIN
entry:
    ld hl,copyright
    MXOS_CALL MXOS_BIOS_PRINT_STRING
    ld c,10
    MXOS_CALL MXOS_BIOS_PRINT_CHAR
    call appProgramDrive
    ld (corePath),a
    ld (registers+7),a
    ld a,1
    ld (registers+5),a
    ld de,NC_BANK_KEY
    ld a,RESIDENT_FIND
    MXOS_CALL MXOS_DOS_RESIDENT
    ld a,h
    or l
    jp nz,selectCore
    ld hl,corePath
    ld a,MODULE_TARGET_BANK
    MXOS_CALL MXOS_DOS_MODULE_LOAD
    jp c,failed
    ld (coreHandle),hl
    ld de,registers
    MXOS_CALL MXOS_DOS_MODULE_CALL
    push af
    ld hl,(coreHandle)
    ld a,1 ; The core retains itself on successful registration.
    MXOS_CALL MXOS_DOS_MODULE_UNLOAD
    pop af
    jp c,failed
    ld hl,(registers+6)
    push hl
    pop af
    jp c,failed
    jp launcherDone
selectCore:
    ld de,NC_BANK_KEY
    ld a,RESIDENT_SHELL_SET
    MXOS_CALL MXOS_DOS_RESIDENT
    jp c,failed
    xor a
    MXOS_CALL MXOS_DOS_SET_SHELL
    jp c,failed
launcherDone:
    xor a
    ret
failed:
    push af
    ld hl,message
    MXOS_CALL MXOS_BIOS_PRINT_STRING
    pop af
    push af
    ld c,a
    MXOS_CALL MXOS_BIOS_PRINT_HEX_BYTE
    ld c,10
    MXOS_CALL MXOS_BIOS_PRINT_CHAR
    MXOS_CALL MXOS_BIOS_GETCH
    pop af
    ret
copyright: DB 'Commander 3.0 BANK (C) Omsk/SPb 1992-2026',0
corePath: DB 'A:',05Ch,'NC_CORE.DLL',0
message: DB 'NC BANK: load/call error ',0
coreHandle: DW 0
registers: DS MX_REGS,0
    INCLUDE "../include/app-drive.inc"
    END
