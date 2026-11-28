; Восстановление командной среды в BIOS после установки IO и MEM.

;----------------------------------------------------------------------------
; MXOS
;
; Запросить путь к оболочке, когда BIOS требуется восстановить командную среду.
;
; вход:
;   Работают BIOS, IO.SYS и файловая служба DOS. Буфер CMD_LINE доступен в основной
;   памяти.
;
; выход:
;   При отказе FILE_EXEC_NATIVE ввод повторяется. После успешного выполнения управление
;   передаётся biosBankShell. Обычного возврата через RET нет.
;----------------------------------------------------------------------------

biosRecoveryStart:
    ld hl,biosShellPrompt
    call printStringZ
    ld hl,CMD_LINE
    ld de,CMD_LINE+MAX_PATH-1
    call input
    ld c,10
    call printChar
    ld hl,CMD_LINE
    MXOS_CALL_REGS MXOS_DOS_FILE_EXEC_NATIVE
    jp c,biosRecoveryStart
    jp biosBankShell
biosShellPrompt: DB 'Shell path: ',0
; The regular shell paths are explicit native images; a missing one returns
; silently instead of badCommand's print/wait path.
biosTryShell:
    push hl
    call printStringZ
    ld c,10
    call printChar
    pop hl
    MXOS_CALL_REGS MXOS_DOS_FILE_EXEC_NATIVE
    ret

; Restore a selected FONT profile only when a destructive launch/Reset marked
; it pending. This executes once before the shell, never during glyph output.
biosRestoreFont:
    ld hl,MAIN_COMMAND_ADDR
    ld a,1
    MXOS_CALL_REGS MXOS_DOS_FONT_CONFIG
    ret c
    or a
    ret z
    ld hl,MAIN_COMMAND_ADDR
    call biosBootExec
    ld a,2
    MXOS_CALL_REGS MXOS_DOS_FONT_CONFIG
    ret
