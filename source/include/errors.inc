; Тексты DOS включаются только в приложения, без строк в ядре.
; A=DOS_ERR_* -> HL=ASCIIZ. AF/DE/HL разрушены, BC сохранён.
dosErrorText:
    cp 13
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
    DW dosErrorCorrupt,dosErrorNoDriver,dosErrorUnsupported,dosErrorBadImage,dosErrorCancelled
    IFDEF DOS_ERROR_UI
dosErrorUnknown: DB 'Unknown error',0
dosErrorIo: DB 'Disk input/output error',0
dosErrorNotFound: DB 'File or path not found',0
dosErrorExists: DB 'Destination already exists',0
dosErrorFull: DB 'Disk is full',0
dosErrorDirFull: DB 'Directory is full',0
dosErrorAccess: DB 'Access denied',0
dosErrorArgument: DB 'Invalid argument',0
dosErrorCorrupt: DB 'File system is corrupted',0
dosErrorNoDriver: DB 'Disk driver is not installed',0
dosErrorUnsupported: DB 'Operation is not supported',0
dosErrorBadImage: DB 'Invalid executable file',0
dosErrorCancelled: DB 'Operation cancelled',0
    ELSE
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
dosErrorCancelled: DB 'OPERATION CANCELLED',0
    ENDIF
