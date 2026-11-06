; Самостоятельная консоль MXOS. Общие команды выполняет DOS в своей странице.
    INCLUDE "../include/mxos.inc"
    ORG 0000h
cliStart:
    call bios_loadDosModule
    jp c,cliModuleError
    ld hl,0
    ld a,CLI_OP_OBSERVER
    MXCALL MX_DOS_CLI
    xor a
    ld (bios_vars.koi7),a
    ld a,2
    MXCALL MX_DOS_SETSHELL
    MXCALL MX_DOS_FILESELECTDRIVE
    call cliHistoryLoad
cliPrompt:
    ld a,1
    MXCALL MX_DOS_SETSHELL
    ld a,DIR_OP_GETCWD
    ld hl,cliPromptPath
    MXCALL MX_DOS_DIRECTORY
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
    MXCALL MX_DOS_CLI
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
    MXCALL MX_DOS_SETSHELL
    ld hl,CMD_LINE
    jp bios_fileExec
cliLeave:
    call cliHistorySave
    jp bios_reboot
cliError:
    MXCALL MX_DOS_GETERROR
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
    MXCALL MX_DOS_SETSHELL
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
