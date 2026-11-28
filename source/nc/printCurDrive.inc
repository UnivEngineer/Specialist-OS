; Буква и двоеточие передаются BIOS одной строкой.
printCurDrive:
            ld e,2
            MXOS_CALL_REGS MXOS_DOS_FILE_GET_SET_DRIVE
printDrive:
            add a,'A'
            ld (ncDriveText),a
            ld hl,ncDriveText
            jp FONT_ENGINE_RAWZ
ncDriveText: DB 'A',':',0
