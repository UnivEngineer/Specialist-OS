; Одноразовый установщик банковой оболочки в свободном участке DOS.
    INCLUDE "../common/system.inc"
    INCLUDE "../common/version.inc"
    INCLUDE "bank-api.inc"
    IFNDEF DRIVER_ORIGIN
DRIVER_ORIGIN = 0
    ENDIF
    ORG DRIVER_ORIGIN
    RELOCATE_START

;----------------------------------------------------------------------------
; MXOS
;
; Сделать банковую версию Commander текущей оболочкой.
;
; вход:
;   Запуск из DOS. Диск из командной строки определяет, где искать NC_CORE.DLL. Ключ
;   NC_BANK_KEY идентифицирует уже зарегистрированную оболочку.
;
; выход:
;   При успехе a = 0, cf = 0. Ядро оболочки загружено и зарегистрировано либо выбран
;   имеющийся резидент. При отказе выводится код, AF ошибки восстанавливается для
;   возврата.
;----------------------------------------------------------------------------

entry:
    ld hl,copyright
    MXOS_CALL_REGS MXOS_BIOS_PRINT_STRING
    ld c,10
    call mx_printChar
    call appProgramDrive
    ld (corePath),a
    ld (registers+7),a
    ld a,1
    ld (registers+5),a
    ld de,NC_BANK_KEY
    ld a,RESIDENT_FIND
    MXOS_CALL_REGS MXOS_DOS_RESIDENT
    ld a,h
    or l
    jp nz,selectCore
    ld hl,corePath
    ld a,MODULE_TARGET_BANK
    MXOS_CALL_REGS MXOS_DOS_MODULE_LOAD
    jp c,failed
    ld (coreHandle),hl
    ld de,registers
    MXOS_CALL_REGS MXOS_DOS_MODULE_CALL
    push af
    ld hl,(coreHandle)
    ld a,1 ; The core retains itself on successful registration.
    MXOS_CALL_REGS MXOS_DOS_MODULE_UNLOAD
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
    MXOS_CALL_REGS MXOS_DOS_RESIDENT
    jp c,failed
    xor a
    MXOS_CALL_REGS MXOS_DOS_SET_SHELL
    jp c,failed
launcherDone:
    xor a
    ret
failed:
    push af
    ld hl,message
    MXOS_CALL_REGS MXOS_BIOS_PRINT_STRING
    pop af
    push af
    ld c,a
    call mx_printHexByte
    ld c,10
    call mx_printChar
    MXOS_CALL_REGS MXOS_BIOS_GETCH
    pop af
    ret
copyright:      DB 'Commander ',NC_VERSION,'.',NC_REVISION,'.',VERSION_BUILD,' BANK (C) Omsk/SPb 1992-2026',0
corePath:       DB 'C:',05Ch,'DOS',05Ch,'NC_CORE.DLL',0
message:        DB 'NC BANK: load/call error ',0
coreHandle:     DW 0
registers:      DS MX_REGS,0
    DEFINE APP_COMMAND_ADDR DOS_BANK_CMD_ADDR
    INCLUDE "../../sdk/asm/lib/app-drive.inc"
    RELOCATE_END
IMAGE_BODY_SIZE  = $-entry
IMAGE_MEM_END    = IMAGE_BODY_SIZE
IMAGE_BSS_SIZE   = 0
IMAGE_RELOC_KIND = IMAGE_RELOC_WORD16
IMAGE_ALIGN_LOG2 = 0
    RELOCATE_TABLE entry
    END
