; Текстовый отчёт строится после всех измерений на месте эталона экрана.
; Открытие, запись и закрытие — штатные fopen/fwrite/fclose через FILE_HANDLE.
REPORT_BUFFER = 06800h
REPORT_LIMIT = 07800h
matrixSaveReport:
    ld hl,reportFailedText
    ld (reportMessage),hl
    ld hl,REPORT_BUFFER
    ld (reportPointer),hl
    xor a
    ld (reportOverflow),a
    ld (reportWriteFailed),a
    inc a
    ld (reportMode),a
    ld hl,title
    call puts
    ld hl,reportNote
    call puts
    ld hl,matrixCases
reportBuildLine:
    ld (matrixCase),hl
    ld a,(hl)
    inc hl
    or (hl)
    jp z,reportBuilt
    call matrixPrintValues
    ld hl,(matrixCase)
    ld de,14
    add hl,de
    jp reportBuildLine
reportBuilt:
    ld hl,(bitmapResult)
    call puts
    xor a
    ld (reportMode),a
    ld a,(reportOverflow)
    or a
    ret nz
    ld hl,(reportPointer)
    ld de,REPORT_BUFFER
    call bios_sub_hl_de
    ld (reportSize),hl
    ld e,0
    call bios_fileGetSetDrive
    ld (reportDrive),a
    ld a,1
    call bios_fileSelectDrive
    jp c,reportRestoreDrive
    ld a,DIR_OP_GET
    call bios_directory
    jp c,reportRestoreDrive
    ex de,hl
    ld (reportDirectory),hl
    ld de,0
    ld a,DIR_OP_SET
    call bios_directory
    jp c,reportRestoreDirectory
    call reportFopen
    jp c,reportRestoreDirectory
    ld hl,REPORT_BUFFER
    ld (reportHandle+FH_BUFFER),hl
    ld hl,(reportSize)
    ld (reportHandle+FH_COUNT),hl
    call reportFwrite
    jp c,reportShortWrite
    ld hl,(reportSize)
    ex de,hl
    ld hl,(reportHandle+FH_COUNT)
    call bios_cmp_hl_de
    jp z,reportClose
reportShortWrite:
    ld a,1
    ld (reportWriteFailed),a
reportClose:
    call reportFclose
    jp c,reportRestoreDirectory
    ld a,(reportWriteFailed)
    or a
    jp nz,reportRestoreDirectory
    ld hl,reportSavedText
    ld (reportMessage),hl
reportRestoreDirectory:
    ld hl,(reportDirectory)
    ex de,hl
    ld a,DIR_OP_SET
    call bios_directory
    jp nc,reportRestoreDrive
    ld hl,reportFailedText
    ld (reportMessage),hl
reportRestoreDrive:
    ld a,(reportDrive)
    call bios_fileSelectDrive
    ret nc
    ld hl,reportFailedText
    ld (reportMessage),hl
    ret

reportFopen:
    ld hl,reportHandle
    ld b,FILE_HANDLE
    xor a
reportClearHandle:
    ld (hl),a
    inc hl
    dec b
    jp nz,reportClearHandle
    ld hl,reportName
    ld de,reportHandle
    ld bc,11
    call bench_copy
    ld a,1
    ld (reportHandle+FH_DRIVE),a
    ld a,FILE_MODE_WRITE | FILE_MODE_CREATE | FILE_MODE_TRUNCATE
    ld (reportHandle+FH_MODE),a
    ld a,FILE_OP_OPEN
    jp reportFileCall
reportFwrite:
    ld a,FILE_OP_WRITE
    jp reportFileCall
reportFclose:
    ld a,FILE_OP_CLOSE
reportFileCall:
    ld hl,reportHandle
    jp bios_fileHandle

; Эти обработчики сохраняют регистры так же, как экранные функции вывода.
reportPuts:
    push af
    push hl
reportPutsLoop:
    ld a,(hl)
    or a
    jp z,reportPutsDone
    call reportPutChar
    inc hl
    jp reportPutsLoop
reportPutsDone:
    pop hl
    pop af
    ret
reportPutChar:
    push af
    push bc
    push hl
    ld c,a
    ld hl,(reportPointer)
    ld a,h
    cp REPORT_LIMIT/256
    jp nc,reportPutOverflow
    ld (hl),c
    inc hl
    ld (reportPointer),hl
    jp reportPutDone
reportPutOverflow:
    ld a,1
    ld (reportOverflow),a
reportPutDone:
    pop hl
    pop bc
    pop af
    ret
