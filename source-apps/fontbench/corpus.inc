; Чтение реального A:\README.TXT вне измерения: CR/LF заменяются пробелом,
; пара CRLF даёт один пробел, в том числе на границе блоков чтения.
; прочие управляющие байты заменяются пробелом, KOI-8 сохраняется побайтно.
; Выход CF=ошибка; AF/BC/DE/HL разрушены. Диск и текущий каталог A: сохранены.
loadCorpus:
    xor a
    ld (corpusStatus),a
    ld e,0
    call bios_fileGetSetDrive
    ld (corpusDrive),a
    xor a
    call bios_fileSelectDrive
    jp c,corpusDriveError
    ld a,DIR_OP_GET
    call bios_directory
    jp c,corpusDriveError
    ex de,hl
    ld (corpusDirectory),hl
    ld de,0
    ld a,DIR_OP_SET
    call bios_directory
    jp c,corpusDirectoryError
    call corpusRead
    jp nc,corpusRestoreDirectory
corpusDirectoryError:
    ld a,1
    ld (corpusStatus),a
corpusRestoreDirectory:
    ld hl,(corpusDirectory)
    ex de,hl
    ld a,DIR_OP_SET
    call bios_directory
    jp nc,corpusRestoreDrive
corpusDriveError:
    ld a,1
    ld (corpusStatus),a
corpusRestoreDrive:
    ld a,(corpusDrive)
    call bios_fileSelectDrive
    jp nc,corpusStatusReady
    ld a,1
    ld (corpusStatus),a
corpusStatusReady:
    ld a,(corpusStatus)
    or a
    ret z
    scf
    ret

corpusRead:
    xor a
    ld (corpusLastCR),a
    ld hl,textCorpus
    ld bc,FONT_CORPUS_SIZE
corpusPad:
    ld (hl),' '
    inc hl
    dec bc
    ld a,b
    or c
    jp nz,corpusPad
    ld hl,textCorpus
    ld (corpusWrite),hl
    ld hl,FONT_CORPUS_SIZE
    ld (corpusRemaining),hl
    ld a,FILE_MODE_READ
    ld (corpusHandle+FH_MODE),a
    ld hl,corpusHandle
    ld a,FILE_OP_OPEN
    call bios_fileHandle
    ret c
corpusChunk:
    ; Временный блок выше эталона; стек услуг 8500h включится лишь после чтения.
    ld hl,08420h
    ld (corpusHandle+FH_BUFFER),hl
    ld hl,256
    ld (corpusHandle+FH_COUNT),hl
    ld hl,corpusHandle
    ld a,FILE_OP_READ
    call bios_fileHandle
    jp c,corpusCloseError
    ld hl,(corpusHandle+FH_COUNT)
    ld b,h
    ld c,l
    ld a,b
    or c
    jp z,corpusClose
    ld hl,(corpusWrite)
    ex de,hl
    ld hl,08420h
corpusByte:
    ld a,(hl)
    inc hl
    call corpusNormalize
    jp c,corpusSkip
corpusPrintable:
    ld (de),a
    inc de
    push hl
    ld hl,(corpusRemaining)
    dec hl
    ld (corpusRemaining),hl
    ld a,h
    or l
    pop hl
    jp z,corpusClose
corpusSkip:
    dec bc
    ld a,b
    or c
    jp nz,corpusByte
    ex de,hl
    ld (corpusWrite),hl
    jp corpusChunk
corpusClose:
    ld hl,corpusHandle
    ld a,FILE_OP_CLOSE
    jp bios_fileHandle
corpusCloseError:
    call corpusClose
    scf
    ret
; A — байт файла; CF=1 пропускает LF после CR. BC/DE/HL сохранены.
corpusNormalize:
    cp 13
    jp z,corpusCR
    cp 10
    jp z,corpusLF
    push af
    xor a
    ld (corpusLastCR),a
    pop af
    cp 020h
    ret nc
    ld a,' '
    or a
    ret
corpusCR:
    ld a,1
    ld (corpusLastCR),a
    ld a,' '
    or a
    ret
corpusLF:
    ld a,(corpusLastCR)
    or a
    jp nz,corpusSkipLF
    ld a,' '
    or a
    ret
corpusSkipLF:
    xor a
    ld (corpusLastCR),a
    scf
    ret
corpusHandle:
    DB 'README  TXT'
    DS FILE_HANDLE-11,0
corpusWrite: DW 0
corpusRemaining: DW 0
corpusDirectory: DW 0
corpusDrive: DB 0
corpusStatus: DB 0
corpusLastCR: DB 0
