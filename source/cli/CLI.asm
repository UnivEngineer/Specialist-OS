; Самостоятельная консоль MXOS. Общие команды выполняет DOSMOD.
    INCLUDE "../include/mxos.inc"
    ORG 0000h
cliStart:
    call bios_loadDosModule
    jp c,cliModuleError
    ld hl,0
    ld a,CLI_OP_OBSERVER
    call bios_cli
    xor a
    ld (bios_vars.koi7),a
    ld a,2
    call bios_setShell
    call bios_fileSelectDrive
    call cliHistoryLoad
cliPrompt:
    ld a,1
    call bios_setShell
    ld a,DIR_OP_GETCWD
    ld hl,cliPromptPath
    call bios_directory
    jp c,cliError
    ld hl,cliPromptPath
    call bios_printString
    ld c,'>'
    call bios_printChar
    call cliInput
    call cliHistoryAdd
    ld c,0Ah
    call bios_printChar
    ld hl,CMD_LINE
    ld de,cliOverwritePrompt
    ld a,CLI_OP_COMMAND
    call bios_cli
    jp c,cliError
    cp 0FFh
    jp z,cliExternal
    ld a,b
    and 2
    jp nz,cliLeave
    jp cliPrompt
cliExternal:
    call cliHistorySave
    ; Нижнее ОЗУ может быть затёрто программой; возврат через загрузку оболочки.
    ld a,1
    call bios_setShell
    ld hl,CMD_LINE
    jp bios_fileExec
cliLeave:
    call cliHistorySave
    jp bios_reboot
cliError:
    call bios_getError
    call dosErrorText
    call bios_printString
    ld c,0Ah
    call bios_printChar
    jp cliPrompt
cliModuleError:
    ld hl,cliModuleErrorText
    call bios_printString
    call bios_getch
    xor a
    call bios_setShell
    jp bios_reboot
cliErrorText: DB 'Error ',0
cliModuleErrorText: DB 'DOS module error',0
cliPromptPath: DS MAX_PATH,0
    INCLUDE "overwrite.inc"
    INCLUDE "../include/errors.inc"
    INCLUDE "input.inc"
    INCLUDE "history.inc"
    ASSERT $ < APP_MEMORY_END
    END
