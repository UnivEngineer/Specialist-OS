; CLI_OP_MOVE: HL=FILE_COPY_REQUEST, область запроса проверяется как у COPY.
; На одном томе буфер данных не используется, FAT и тело файла не меняются.
; Между томами COPY проверяет явные buffer/bufferSize, затем удаляется источник.
; AF/BC/DE/HL разрушены, CF/A=статус DOS. Существующее назначение запрещено.
cliMoveRequest:
    ld bc,FILE_COPY_REQUEST
    call fileOpRange
    ret c
    push hl
    ld de,FILE_COPY_REQUEST.sourceDrive
    add hl,de
    ld a,(hl)
    inc hl
    cp (hl)
    pop hl
    jp z,cliMoveRequestSame
    ; Общий COPY проверяет и буфер, и его пересечение с запросом.
    push hl
    call fileCopyForMove
    pop hl
    ret c
    ld a,(copySkipped)
    or a
    jp nz,cliSuccess
    ld a,(treeHandled)
    or a
    jp nz,cliSuccess
    ld a,1
    jp cliMoveRequestCopy
cliMoveRequestSame:
    xor a
cliMoveRequestCopy:
    ld (cliMoveFlag),a
    ld bc,FILE_COPY_REQUEST
    ld de,cliRequest
    call cliCopyBytes
    call cliRemember
    ld a,(cliMoveFlag)
    or a
    jp nz,cliMoveRequestDelete
    call cliMovePrepared
    jp cliFileDone
cliMoveRequestDelete:
    call cliMoveDelete
    jp cliFileDone
cliMovePrepared:
    ld a,(cliRequest.sourceDrive)
    ld b,a
    ld a,(cliRequest.destinationDrive)
    cp b
    jp nz,cliMoveAcross
    ld a,b
    call fileChangeDrive
    ret c
    ld hl,(cliRequest.sourceDirectory)
    ld (v_directory),hl
    ld hl,cliRequest.source
    ld a,(hl)
    cp '.'
    jp z,cliMoveDenied
    call fileFindAny
    ret c
    ld hl,v_tmpFileDescr
    ld de,cliRequest.source
    ld bc,FILE_DESCR_SIZE
    call cliCopyBytes
    ld a,(cliRequest.source+FILE_DESCRIPTOR.attrib)
    and 5
    jp nz,cliMoveDenied
    ; Сохраняем метаданные источника, имя берётся из назначения.
    ld hl,cliRequest.source+11
    ld de,cliRequest.destination+11
    ld bc,FILE_DESCR_SIZE-11
    call cliCopyBytes
    ld hl,(cliRequest.sourceDirectory)
    ex de,hl
    ld hl,(cliRequest.destinationDirectory)
    call cmp_hl_de
    jp nz,cliMoveOtherDirectory
    ld hl,cliRequest.source
    ld de,cliRequest.destination
    ld c,11
    call stricmp
    jp z,cliMoveRename
    ld hl,cliRequest.destination
    call fileFindAny
    jp nc,cliMoveExists
    ld a,(v_dosError)
    cp DOS_ERR_NOT_FOUND
    jp nz,cliMoveError
cliMoveRename:
    ld hl,cliRequest.source
    ld de,cliRequest.destination
    call cliChangedObject
    jp fatFileRename
cliMoveOtherDirectory:
    call cliMoveCheckParent
    ret c
    ld hl,(cliRequest.destinationDirectory)
    ld (v_directory),hl
    ld hl,cliRequest.destination
    call fileFindAny
    jp nc,cliMoveExists
    ld a,(v_dosError)
    cp DOS_ERR_NOT_FOUND
    jp nz,cliMoveError
    call fatDirStart
    ret c
cliMoveSlot:
    call fatDirNext
    jp c,cliMoveFull
    ld a,(hl)
    or a
    jp z,cliMovePublish
    cp 0E5h
    jp nz,cliMoveSlot
cliMovePublish:
    call cliChanged
    call fatDirPrepareEntry
    ret c
    call markDirSectorAsModified
    ret c
    ex de,hl
    ld hl,cliRequest.destination
    ld bc,FILE_DESCR_SIZE
    call cliCopyBytes
    ld hl,(v_cachedDescrPtr)
    call fatEncodeName
    call saveSectorCache
    ret c
    call fatFinishWrite
    ret c
    call cliMoveFixParent
    ret c
    ; Только после сохранения новой записи помечаем старую удалённой.
    ld hl,(cliRequest.sourceDirectory)
    ld (v_directory),hl
    ld hl,cliRequest.source
    call fileFindAny
    ret c
    call markDirSectorAsModified
    ret c
    call cliChangedObject
    ld (hl),0E5h
    call saveSectorCache
    ret c
    jp fatFinishWrite
cliMoveFull:
    or a
    jp nz,cliMoveError
    call directoryExtend
    ret c
    jp cliMovePublish
cliMoveExists:
    ld a,DOS_ERR_EXISTS
    jp dosSetError
cliMoveDenied:
    ld a,DOS_ERR_ACCESS
    jp dosSetError
cliMoveError:
    scf
    ret
cliMoveAcross:
    ; Текстовый CLI использует свой буфер; публичный запрос обработан выше.
    call cliMoveCopyInternal
cliMoveCopied:
    ret c
    ld a,(copySkipped)
    or a
    jp nz,cliSuccess
    ld a,(treeHandled)
    or a
    jp nz,cliSuccess
cliMoveDelete:
    ld a,(cliRequest.sourceDrive)
    call fileChangeDrive
    ret c
    ld hl,(cliRequest.sourceDirectory)
    ld (v_directory),hl
    ld hl,cliRequest.source
    jp cliDeletePrepared
