; F7 — создать каталог через общий CLI, путь ограничен MAX_PATH.
but_F7:
            call drawWindow1
            CENTER_LINE 14, 5Eh
            ld hl,ncMkdirText
            call bios_printString
            xor a
            ld (vars.input),a
            ld de,vars.input
            call input
            cp 01Bh
            jp z,ncRefreshDirty
            ld hl,vars.input
            ld a,CLI_OP_MKDIR
            MXCALL MX_DOS_CLI
            jp c,copyFileError
            jp ncRefreshDirty
ncMkdirText: DB 'Make directory',0
