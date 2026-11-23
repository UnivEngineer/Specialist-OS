; The same postorder tree walker serves cross-volume MOVE and DELETE.
foDeleteSync:
    push hl
    call foContextSave
    pop hl
    xor a
    ld (foAsync),a
    call foDelete
    push af
    call foContextRestore
    pop af
    ret
foDelete:
    ld (v_copyRequest),hl
    call foRootInfo
    ret c
    ld hl,(v_copyRequest)
    ld de,cliRequest
    ld bc,FILE_COPY_REQUEST
    call cliCopyBytes
    ld a,(cliRequest.source)
    cp '.'
    jp z,foDeleteDenied
    ld a,(cliRequest.source+FILE_DESCRIPTOR.attrib)
    and 5
    jp nz,foDeleteDenied
    ld a,(cliRequest.source+FILE_DESCRIPTOR.attrib)
    and 10h
    jp z,foDeleteFile
    ; Never remove a directory containing this drive's saved CWD.
    ld a,(v_drive)
    add a,a
    ld e,a
    ld d,0
    ld hl,foCallerDirectories
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld (cliRequest+FILE_COPY_REQUEST.destinationDirectory),hl
    call cliMoveCheckParent
    ret c
    ld a,FILEOPS_DELETE
    ld (treeMoveMode),a
    ld a,(foCallerDrive)
    push af
    jp treeCopyStart
foDeleteDenied:
    ld a,DOS_ERR_ACCESS
    jp dosSetError
foDeleteFile:
    ld hl,(v_copyRequest)
    ld e,FILE_COPY_REQUEST.sourceDrive
    call fc_Select
    ret c
    ld e,FILE_COPY_REQUEST.source
    call fc_Address
    call cliDeletePrepared
    ret c
    ld a,(foAsync)
    or a
    ret z
    call foFileDone
    ld hl,(v_copyRequest)
    ld de,FILE_COPY_REQUEST.source+FILE_DESCRIPTOR.size
    add hl,de
    ld de,foJob.bytes
    jp fpAdd32

; Stat the root into the caller's request; build a path without exposing DOS.
foRootInfo:
    ld e,FILE_COPY_REQUEST.sourceDrive
    call fc_Select
    ret c
    ld e,FILE_COPY_REQUEST.source
    call fc_Address
    call fileFindAny
    ret c
    call fileLoadExit
    push hl
    ld hl,(v_directory)
    push hl
    ld hl,foPathText
    call directoryCwdUnchecked
    pop hl
    ld (v_directory),hl
    pop hl
    ret c
    push hl
    call fpPathEnd
    ld a,(de)
    ; GETCWD has no trailing separator except in root.
    dec de
    ld a,(de)
    inc de
    cp 05Ch
    jp z,fpRootSlash
    ld a,05Ch
    call fpPut
fpRootSlash:
    pop hl
    ret c
    jp fpName
foGetPath:
    ld hl,foPathText
    ld bc,0
fpPathCount:
    inc c
    ld a,(hl)
    inc hl
    or a
    jp nz,fpPathCount
    ld hl,foPathText
    ld a,(foPathStamp)
    ret
foTreePrefix:
    call fpPathEnd
    ld a,05Ch
    call fpPut
    ret c
    xor a
    ld (de),a
    ld a,e
    sub foPathText & 0FFh
    ld hl,(treeFrame)
    ld de,7
    add hl,de
    ld (hl),a
    ret
foTreePath:
    ld hl,(treeFrame)
    ld de,7
    add hl,de
    ld e,(hl)
    ld d,0
    ld hl,foPathText
    add hl,de
    ex de,hl
    ld hl,treeRequest.source
    jp fpName
fpPathEnd:
    ld de,foPathText
fpPathEndLoop:
    ld a,(de)
    or a
    ret z
    inc de
    jp fpPathEndLoop
fpName:
    push hl
    ld b,8
    call fpNamePart
    pop hl
    ret c
    push de
    ld de,8
    add hl,de
    pop de
    ld a,(hl)
    cp ' '
    jp z,fpNameEnd
    ld a,'.'
    call fpPut
    ret c
    ld b,3
    call fpNamePart
    ret c
fpNameEnd:
    xor a
    ld (de),a
    ld hl,foPathStamp
    inc (hl)
    or a
    ret
fpNamePart:
    ld a,(hl)
    inc hl
    cp ' '
    jp z,fpNamePartDone
    call fpPut
    ret c
    dec b
    jp nz,fpNamePart
fpNamePartDone:
    or a
    ret
fpPut:
    push hl
    push af
    ld hl,foPathText+MAX_PATH-1
    ld a,h
    cp d
    jp nz,fpPutHigh
    ld a,l
    cp e
fpPutHigh:
    jp z,fpPutFull
    jp c,fpPutFull
    pop af
    ld (de),a
    inc de
    pop hl
    or a
    ret
fpPutFull:
    pop af
    pop hl
    jp blockArgument
foPathStamp:    DB 0
foPathText:     DS MAX_PATH,0

