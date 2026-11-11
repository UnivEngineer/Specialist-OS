; Контроллер main: обычная загрузка COM, все модули — через публичный ABI.
    INCLUDE "../../source/include/mxos.inc"
IMAGE_FLAGS = IMAGE_FLAG_MAIN
    ORG 0
entry:
    ld a,(bios_upper.koi7)
    push af
    xor a
    call bios_setFontEncoding
    ld hl,06000h
    ld de,02000h
    ld b,0
    ld a,MEM_OP_ARENA
    MXOS_CALL MXOS_DOS_MEMORY
    jp c,failed
    ld hl,bankPath
    ld a,MODULE_TARGET_BANK
    MXOS_CALL MXOS_DOS_MODULE_LOAD
    jp c,failed
    ld (handle),hl
    ld de,regs
    MXOS_CALL MXOS_DOS_MODULE_CALL
    jp c,failed
    ld hl,(regs)
    ld de,01235h
    call bios_cmp_hl_de
    jp nz,failed
    ld hl,(handle)
    MXOS_CALL MXOS_DOS_MODULE_UNLOAD
    jp c,failed
    ld hl,mainPath
    ld a,MODULE_TARGET_MAIN
    MXOS_CALL MXOS_DOS_MODULE_LOAD
    jp c,failed
    ld (handle),hl
    ld de,regs
    MXOS_CALL MXOS_DOS_MODULE_CALL
    jp c,failed
    ld hl,(handle)
    MXOS_CALL MXOS_DOS_MODULE_UNLOAD
    jp c,failed
    ld hl,dosPath
    ld a,MODULE_TARGET_DOS
    MXOS_CALL MXOS_DOS_MODULE_LOAD
    jp c,failed
    ld (handle),hl
    ld de,regs
    MXOS_CALL MXOS_DOS_MODULE_CALL
    jp c,failed
    ld hl,(handle)
    MXOS_CALL MXOS_DOS_MODULE_UNLOAD
    jp c,failed
passed:
    ld hl,055AAh
    ld (testResult),hl
    ld hl,ok
    MXOS_CALL MXOS_BIOS_PRINT_STRING
    jp finish
failed:
    ld hl,0DEADh
    ld (testResult),hl
    ld hl,error
    MXOS_CALL MXOS_BIOS_PRINT_STRING
finish:
    call bios_getch
    pop af
    call bios_setFontEncoding
    ret
testResult: DW 0
handle: DW 0
regs: DW 01234h,02345h,03456h,0
bankPath: DB 'B:',05Ch,'BANKMOD.DLL',0
    DS MAX_PATH-($-bankPath),0
mainPath: DB 'B:',05Ch,'MAINMOD.DLL',0
    DS MAX_PATH-($-mainPath),0
dosPath: DB 'B:',05Ch,'DOSMOD.DLL',0
    DS MAX_PATH-($-dosPath),0
ok: DB 'Module API PASS',13,10,0
error: DB 'Module API FAIL',13,10,0
