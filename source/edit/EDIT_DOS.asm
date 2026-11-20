; One-shot DOS launcher for the banked editor.
    INCLUDE "../include/mxos.inc"
    IFNDEF DRIVER_ORIGIN
DRIVER_ORIGIN = DOS_BANK_DRIVER_ADDR
    ENDIF
    ORG DRIVER_ORIGIN
entry:
    ex de,hl
    ld (registers+2),hl
    ld a,DOS_BANK_PAGE
    ld (registers+4),a
    call appProgramDrive
    ld (corePath),a
    ld (registers+5),a           ; B = drive containing the editor DLL
    ld hl,corePath
    ld a,MODULE_TARGET_BANK
    MXOS_CALL MXOS_DOS_MODULE_LOAD
    jp c,failed
    ld (coreHandle),hl
    ld de,registers
    MXOS_CALL MXOS_DOS_MODULE_CALL
    jp c,unload
    ld hl,(registers+6)
    push hl
    pop af
unload:
    push af
    ld hl,(coreHandle)
    xor a
    MXOS_CALL MXOS_DOS_MODULE_UNLOAD
    pop af
    ret nc
failed:
    push af
    ld hl,error
    MXOS_CALL MXOS_BIOS_PRINT_STRING
    pop af
    push af
    ld c,a
    MXOS_CALL MXOS_BIOS_PRINT_HEX_BYTE
    ld hl,prompt
    MXOS_CALL MXOS_BIOS_PRINT_STRING
waitEnter:
    MXOS_CALL MXOS_BIOS_GETCH
    cp 13
    jp nz,waitEnter
    pop af
    ret
corePath: DB 'A:',05Ch,'EDITCORE.DLL',0
error: DB 'EDIT BANK: load/call error ',0
prompt: DB 13,10,'Press Enter to return.',13,10,0
coreHandle: DW 0
registers: DS MX_REGS,0
    INCLUDE "../include/app-drive.inc"
IMAGE_BSS_SIZE = 0
IMAGE_MEM_END = $
IMAGE_MEMORY_POLICY = IMAGE_POLICY_MANAGED
IMAGE_API = 2
