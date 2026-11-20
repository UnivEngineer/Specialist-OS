; Буква и двоеточие передаются BIOS одной строкой.
printCurDrive:
            ld e,2
            MXOS_CALL MXOS_DOS_FILE_GET_SET_DRIVE
printDrive:
            add a,'A'
            ld (ncDriveText),a
            ld hl,ncDriveText
            jp bios_printStringReal
ncDriveText: DB 'A',':',0
