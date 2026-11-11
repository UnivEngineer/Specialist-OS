; Самостоятельная консоль MXOS. Общие команды выполняет DOS в своей странице.
    INCLUDE "../include/mxos.inc"
IMAGE_FLAGS = IMAGE_FLAG_MAIN
    ORG 0000h
cliStart:
    ld hl,0
    ld a,CLI_OP_OBSERVER
    MXOS_CALL MXOS_DOS_CLI
    xor a
    call bios_setFontEncoding
    ld a,2
    MXOS_CALL MXOS_DOS_SET_SHELL
    MXOS_CALL MXOS_DOS_FILE_SELECT_DRIVE
    call cliHistoryLoad
cliPrompt:
    ld a,1
    MXOS_CALL MXOS_DOS_SET_SHELL
    ld a,DIR_OP_GETCWD
    ld hl,cliPromptPath
    MXOS_CALL MXOS_DOS_DIRECTORY
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
    MXOS_CALL MXOS_DOS_CLI
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
    MXOS_CALL MXOS_DOS_SET_SHELL
    ld hl,CMD_LINE
    ; После RET приложения код CLI в main уже может быть затёрт.
    ld de,bios_reboot
    push de
    jp bios_fileExec
cliLeave:
    call cliHistorySave
    jp bios_reboot
cliError:
    MXOS_CALL MXOS_DOS_GET_ERROR
    call dosErrorText
    call bios_printString
    ld c,0Ah
    call bios_printChar
    jp cliPrompt
cliErrorText: DB 'ERROR ',0
cliPromptPath: DS MAX_PATH,0
    INCLUDE "overwrite.inc"
    INCLUDE "../include/errors.inc"
    INCLUDE "input.inc"
    INCLUDE "history.inc"
    ASSERT $ < APP_MEMORY_END
    END
