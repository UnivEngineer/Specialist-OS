; Буква и двоеточие передаются BIOS одной строкой.
printCurDrive:
            ld e,2
            call bios_fileGetSetDrive
printDrive:
            add a,'A'
            ld (ncDriveText),a
            ld hl,ncDriveText
            jp bios_printStringReal
ncDriveText: DB 'A',':',0
