; Одноразовый загрузчик банкового редактора в свободном участке DOS.
    INCLUDE "../include/mxos.inc"
    INCLUDE "../include/version.inc"
    IFNDEF DRIVER_ORIGIN
DRIVER_ORIGIN = 0
    ENDIF
    ORG DRIVER_ORIGIN
    RELOCATE_START

;----------------------------------------------------------------------------
; MXOS
;
; Запустить банковую версию редактора через EDITCORE.DLL.
;
; вход:
;   de = адрес аргументов в странице DOS. Командная строка содержит диск запуска для
;   поиска DLL.
;
; выход:
;   При успешной загрузке и вызове возвращается AF обработчика редактора после попытки
;   выгрузки. При отказе загрузки или вызова печатается код, после Enter
;   восстанавливается AF ошибки.
;----------------------------------------------------------------------------

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
    MXOS_CALL_REGS MXOS_DOS_MODULE_LOAD
    jp c,failed
    ld (coreHandle),hl
    ld de,registers
    MXOS_CALL_REGS MXOS_DOS_MODULE_CALL
    jp c,unload
    ld hl,(registers+6)
    push hl
    pop af
unload:
    push af
    ld hl,(coreHandle)
    xor a
    MXOS_CALL_REGS MXOS_DOS_MODULE_UNLOAD
    pop af
    ret nc
failed:
    push af
    ld hl,error
    MXOS_CALL_REGS MXOS_BIOS_PRINT_STRING
    pop af
    push af
    ld c,a
    call mx_printHexByte
    ld hl,prompt
    MXOS_CALL_REGS MXOS_BIOS_PRINT_STRING
waitEnter:
    MXOS_CALL_REGS MXOS_BIOS_GETCH
    cp 13
    jp nz,waitEnter
    pop af
    ret
corePath:       DB 'C:',05Ch,'DOS',05Ch,'EDITCORE.DLL',0
error:          DB 'EDIT ',EDIT_VERSION,'.',EDIT_REVISION,'.',VERSION_BUILD,' BANK: load/call error ',0
prompt:         DB 13,10,'Press Enter to return.',13,10,0
coreHandle:     DW 0
registers:      DS MX_REGS,0
    INCLUDE "../include/app-drive.inc"
    RELOCATE_END
IMAGE_BODY_SIZE     = $-entry
IMAGE_BSS_SIZE      = 0
IMAGE_MEM_END       = IMAGE_BODY_SIZE
IMAGE_MEMORY_POLICY = IMAGE_POLICY_MANAGED
IMAGE_API           = 2
IMAGE_RELOC_KIND    = IMAGE_RELOC_WORD16
IMAGE_ALIGN_LOG2    = 0
    RELOCATE_TABLE entry
