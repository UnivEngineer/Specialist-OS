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
dosErrorUnknown: DB 'UNKNOWN ERROR',0
dosErrorIo: DB 'DISK I/O ERROR',0
dosErrorNotFound: DB 'FILE OR PATH NOT FOUND',0
dosErrorExists: DB 'DESTINATION EXISTS',0
dosErrorFull: DB 'DISK FULL',0
dosErrorDirFull: DB 'DIRECTORY FULL',0
dosErrorAccess: DB 'ACCESS DENIED',0
dosErrorArgument: DB 'INVALID ARGUMENT',0
dosErrorCorrupt: DB 'CORRUPT FILESYSTEM',0
dosErrorNoDriver: DB 'NO DISK DRIVER',0
dosErrorUnsupported: DB 'UNSUPPORTED OPERATION',0
dosErrorBadImage: DB 'INVALID EXECUTABLE',0
