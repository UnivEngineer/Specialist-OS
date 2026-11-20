; Общий CLI. Нереентерабелен, как DOS. Парсер и его временные данные резидентны в DOS.
; HL=строка; CF/A=ошибка, A=FFh/CF=0 — внешняя команда.
cliService:
    or a
    jp z,cliTextService
    dec a
    jp z,fileCopy
    dec a
    jp z,cliRenamePrepared
    dec a
    jp z,cliDeleteTree
    dec a
    jp z,directoryCreate
    dec a
    jp z,directoryPathUnchecked
    dec a
    jp z,directoryRemove
    dec a
    jp z,cliMoveRequest
    dec a
    jp z,cliQueryService
    dec a
    jp z,cliSetObserver
    dec a
    jp z,cliAssociationQuery
    jp blockArgument

cliAssociationQuery:
    ld de,cliAssociationBuffer
    ld bc,512
    call fileAssociation
    ex de,hl                ; borrowed command in the DOS bank, through CR/FF
    ret

    INCLUDE "commands.inc"
    INCLUDE "attrib.inc"

cliSuccess:
    xor a
    ld (v_dosError),a
    ret

cliRemember:
    push hl
    call selectDirectory
    ld hl,v_directories
    ld de,cliSavedDirectories
    ld bc,16
    call cliCopyBytes
    ld a,(v_drive)
    ld (cliOldDrive),a
    ld a,(v_diskInfo.isValid)
    ld (cliOldValid),a
    ld hl,(v_directory)
    ld (cliOldDirectory),hl
    pop hl
    ret
cliFileDone:
    jp nc,cliFileOk
    call dosIoError
    jp cliFileStatus
cliFileOk:
    xor a
    ld (v_dosError),a
cliFileStatus:
    ld a,(v_dosError)
    push af
    ld hl,cliSavedDirectories
    ld de,v_directories
    ld bc,16
    call cliCopyBytes
    ld a,(cliOldDrive)
    call fileChangeDrive
    pop bc
    jp nc,cliRestoreStatus
    ld a,b
    or a
    jp nz,cliRestoreStatus
    ; Ранее нераспознанный том не должен запрещать команду на другом диске.
    ; Ошибка flush до фактического выбора прежнего диска остаётся ошибкой.
    ld a,(cliOldDrive)
    ld hl,v_drive
    cp (hl)
    jp nz,dosIoError
    ld a,(cliOldValid)
    cp DISK_VALID
    jp nz,cliRestoreStatus
    jp dosIoError
cliRestoreStatus:
    ld a,b
    ld (v_dosError),a
    or a
    ret z
    scf
    ret

cliFileExists:
    ld a,DOS_ERR_EXISTS
    call dosSetError
    jp cliFileDone
cliFileArgument:
    call blockArgument
    jp cliFileDone

cliChunk:
    ld bc,256
    ld a,h
    or a
    jp nz,cliChunkReady
    ld b,h
    ld c,l
cliChunkReady:
    ld a,l
    sub c
    ld l,a
    ld a,h
    sbc a,b
    ld h,a
    ld (cliRemaining),hl
    ret
cliCopyBytes:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,cliCopyBytes
    ret
; Поиск COM/EXE/BAT: текущий каталог, затем каталоги PATH.
; Явный путь обхода PATH не вызывает; буферы не обрезаются.

cliCopyInternal:
    xor a
    jp cliCopyModeInternal
cliMoveCopyInternal:
    ld a,1
cliCopyModeInternal:
    IFDEF BANKED_DOS_BUILD
    push af
    ld hl,KD_DATA
    ld (cliRequest.buffer),hl
    ld hl,KD_CHUNK
    ld (cliRequest.bufferSize),hl
    pop af
    ENDIF
    add a,2
    ld hl,cliRequest
    jp fileopsRun
cliStream:
    push af
    ld hl,(cliCursor+2)
    ld (v_streamOffset),hl
    ld hl,(cliCursor)
    ex de,hl
    ld hl,cliIoBuffer
    pop af
    call fatStream
    push af
    ex de,hl
    ld (cliCursor),hl
    ld hl,(v_streamOffset)
    ld (cliCursor+2),hl
    pop af
    ret

; Только голое имя может использовать системный каталог поиска.
