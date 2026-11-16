; F7 — создать каталог через общий CLI, путь ограничен MAX_PATH.
but_F7:
            call drawWindow1
            ld hl,ncMkdirText
            ld e,94
            call ncPrintCentered
            xor a
            ld (vars.input),a
            ld de,vars.input
            call input
            cp 01Bh
            jp z,ncRefreshDirty
            ld hl,vars.input
            ld a,CLI_OP_MKDIR
            MXOS_CALL MXOS_DOS_CLI
            jp c,copyFileError
            jp ncRefreshDirty
ncMkdirText: DB 'Make directory',0
