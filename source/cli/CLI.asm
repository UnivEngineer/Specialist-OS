; Самостоятельная консоль MXOS. Общие команды выполняет DOS в своей странице.
    INCLUDE "../common/system.inc"
IMAGE_FLAGS         = IMAGE_FLAG_MAIN
IMAGE_MEMORY_POLICY = IMAGE_POLICY_TRANSIENT
    ORG 0000h

;----------------------------------------------------------------------------
; MXOS
;
; Войти в основную консоль MXOS и запустить цикл чтения команд.
;
; вход:
;   Запущена CLI.COM в MAIN, доступны BIOS и файловые службы DOS. Аргументов в
;   регистрах не требуется.
;
; выход:
;   При EXIT сохраняется история и управление передаётся bios_reboot. Внешняя программа
;   получает строку из CMD_LINE и собственный путь возврата в оболочку. Сохранённый код
;   отказа FILE_EXEC печатается один раз перед первым приглашением.
;----------------------------------------------------------------------------

cliStart:
    ld hl,0
    ld a,CLI_OP_OBSERVER
    MXOS_CALL_REGS MXOS_DOS_CLI
    xor a
    call bios_setFontEncoding
    ld a,2
    MXOS_CALL_REGS MXOS_DOS_SET_SHELL
    MXOS_CALL_REGS MXOS_DOS_FILE_SELECT_DRIVE
    ld hl,cliFileEnd
    ld bc,IMAGE_BSS_SIZE
cliClearBss:
    ld (hl),0
    inc hl
    dec bc
    ld a,b
    or c
    jp nz,cliClearBss
    call cliHistoryLoad
    ld hl,bios_cliExecError
    ld a,(hl)
    ld (hl),0
    or a
    jp nz,cliShowError
cliPrompt:
    ld a,1
    MXOS_CALL_REGS MXOS_DOS_SET_SHELL
    ld a,DIR_OP_GETCWD
    ld hl,cliPromptPath
    MXOS_CALL_REGS MXOS_DOS_DIRECTORY
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
    MXOS_CALL_REGS MXOS_DOS_CLI
    jp c,cliError
    cp 0FFh
    jp z,cliExternal
    ld a,b
    and 2
    jp nz,cliLeave
    jp cliPrompt
cliExternal:
    call cliHistorySave
    ld a,1
    MXOS_CALL_REGS MXOS_DOS_SET_SHELL
    ; Внешняя команда может занять MAIN. BIOS загрузит CLI заново.
    ld hl,CMD_LINE
    MXOS_EXEC bios_cliExecDone
cliLeave:
    call cliHistorySave
    jp bios_reboot

;----------------------------------------------------------------------------
; MXOS
;
; Показать сообщение об отказе и вывести очередное приглашение CLI.
;
; вход:
;   cliError читает последнюю ошибку DOS, cliShowError принимает её код в a.
;
; выход:
;   Строка сообщения и перевод строки выводятся в консоль. Управление переходит
;   на cliPrompt, обычного возврата через RET нет. Все регистры рабочие.
;----------------------------------------------------------------------------
cliError:
    MXOS_CALL_REGS MXOS_DOS_GET_ERROR
cliShowError:
    call dosErrorText
    call bios_printStringRawZ
    ld c,0Ah
    call bios_printChar
    jp cliPrompt
cliPromptPath: DS MAX_PATH,0
    INCLUDE "overwrite.inc"
    DEFINE DOS_ERRORS_DRIVE BOOT_VOLUME_LETTER
    INCLUDE "../../sdk/asm/lib/errors.inc"
    INCLUDE "input.inc"
    INCLUDE "history.inc"
    INCLUDE "../../sdk/asm/lib/file-io.inc"
cliFileEnd:
; Черновик и кольцо находятся в BSS после кода и рабочих дескрипторов.
cliHistoryDraft   = cliFileEnd
cliHistoryRecord  = cliHistoryDraft+MAX_PATH
cliHistoryData    = cliHistoryRecord+MAX_PATH
cliHistoryCount   = cliHistoryData+4
cliHistoryNext    = cliHistoryCount+1
cliHistoryEntries = cliHistoryNext+1
cliHistoryEnd     = cliHistoryEntries+CLI_HISTORY_COUNT*MAX_PATH
IMAGE_BSS_SIZE = cliHistoryEnd-cliFileEnd
IMAGE_MEM_END  = cliHistoryEnd
    ASSERT IMAGE_MEM_END < APP_MEMORY_END
    END
