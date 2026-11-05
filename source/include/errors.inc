; Тексты DOS включаются только в приложения, без строк в ядре.
; A=DOS_ERR_* -> HL=ASCIIZ. AF/DE/HL разрушены, BC сохранён.
dosErrorText:
    cp 12
    jp c,dosErrorKnown
    xor a
dosErrorKnown:
    add a,a
    ld e,a
    ld d,0
    ld hl,dosErrorTable
    add hl,de
    ld a,(hl)
    inc hl
    ld h,(hl)
    ld l,a
    ret
dosErrorTable:
    DW dosErrorUnknown,dosErrorIo,dosErrorNotFound,dosErrorExists
    DW dosErrorFull,dosErrorDirFull,dosErrorAccess,dosErrorArgument
    DW dosErrorCorrupt,dosErrorNoDriver,dosErrorUnsupported,dosErrorBadImage
dosErrorUnknown: DB 'Unknown error',0
dosErrorIo: DB 'Disk I/O error',0
dosErrorNotFound: DB 'File or path not found',0
dosErrorExists: DB 'Destination exists',0
dosErrorFull: DB 'Disk full',0
dosErrorDirFull: DB 'Directory full',0
dosErrorAccess: DB 'Access denied',0
dosErrorArgument: DB 'Invalid argument',0
dosErrorCorrupt: DB 'Corrupt filesystem',0
dosErrorNoDriver: DB 'No disk driver',0
dosErrorUnsupported: DB 'Unsupported operation',0
dosErrorBadImage: DB 'Invalid executable',0
