; MKDIR/RMDIR — файловые операции DOS, реализация в загружаемом модуле.
; HL=путь MAX_PATH. CF/A=код DOS, исходный диск/CWD сохраняется.
; AF/BC/DE/HL разрушены. Только пустые каталоги, без рекурсивного удаления.
directoryCreate:
    call directoryValidateLeaf
    ret c
    push hl
    ld hl,cliRequest.source
    ld bc,FILE_DESCR_SIZE
    call cliZero
    pop hl
    call cliRemember
    ld de,cliRequest.source
    call fileNamePrepare
    jp c,cliFileDone
directoryCreatePrepared:
    ld hl,(v_directory)
    ld (cliParent),hl
    ld hl,cliRequest.source
    call fileFindAny
    jp nc,cliFileExists
    ld a,(v_dosError)
    cp DOS_ERR_NOT_FOUND
    jp nz,cliFileDone
    ; Создать цепочку одного кластера как временный файл, до публикации DIR.
    ld hl,1
    ld (cliRequest.source+FILE_DESCRIPTOR.size),hl
    ld hl,0
    ld (cliRequest.source+FILE_DESCRIPTOR.size+2),hl
    ld hl,cliRequest.source
    call cliChanged
    call fileCreatePrepare
    jp c,cliFileDone
    ld hl,(cliRequest.source+FILE_DESCRIPTOR.firstCluster)
    ld (cliCursor),hl
    ld (cliNewCluster),hl
    ld hl,0
    ld (cliCursor+2),hl
    ld hl,cliIoBuffer
    ld bc,256
    call cliZero
    ld hl,cliIoBuffer
    ld b,11
    ld a,' '
directoryDotName:
    ld (hl),a
    inc hl
    dec b
    jp nz,directoryDotName
    ld a,'.'
    ld (cliIoBuffer),a
    ld hl,cliIoBuffer
    ld de,cliIoBuffer+32
    ld bc,32
    call cliCopyBytes
    ld a,'.'
    ld (cliIoBuffer+33),a
    ld a,010h
    ld (cliIoBuffer+11),a
    ld (cliIoBuffer+43),a
    ld hl,(cliNewCluster)
    ld (cliIoBuffer+26),hl
    ld hl,(cliParent)
    ld (cliIoBuffer+58),hl
    ld hl,(v_diskInfo.clusterBytes)
    ld (cliRemaining),hl
directoryInitLoop:
    ld hl,(cliRemaining)
    ld a,h
    or l
    jp z,directoryPublish
    call cliChunk
    ld de,cliCursor
    ld hl,cliIoBuffer
    ld a,1
    call cliStream
    jp c,directoryCreateRollback
    ld hl,cliIoBuffer
    ld bc,64
    call cliZero
    jp directoryInitLoop
directoryPublish:
    ld hl,cliRequest.source
    call fileFindAny
    jp c,directoryCreateRollback
    call markDirSectorAsModified
    jp c,directoryCreateRollback
    ld de,11
    add hl,de
    ld (hl),010h
    ld de,17
    add hl,de
    ld b,4
    xor a
directoryZeroSize:
    ld (hl),a
    inc hl
    dec b
    jp nz,directoryZeroSize
    call saveSectorCache
    call fatFinishWrite
    jp cliFileDone
directoryCreateRollback:
    ld a,(v_dosError)
    push af
    ld hl,cliRequest.source
    call fileDelete
    pop af
    call dosSetError
    jp cliFileDone
; Сначала проверяем содержимое и атрибуты, затем удаляем запись и цепочку.
directoryRemove:
    call cliRemember
    ld de,cliRequest.source
    call fileNamePrepare
    jp c,cliFileDone
    ld hl,(v_directory)
    ld (cliParent),hl
    ld hl,cliRequest.source
    ld a,(hl)
    cp '.'
    jp z,cliDirectoryDenied
    call fileFindAny
    jp c,cliFileDone
    ld a,(v_tmpFileDescr+FILE_DESCRIPTOR.attrib)
    and 010h
    jp z,cliFileArgument
    ld a,(v_tmpFileDescr+FILE_DESCRIPTOR.attrib)
    and 005h
    jp nz,cliDirectoryDenied
    ld hl,(v_tmpFileDescr+FILE_DESCRIPTOR.firstCluster)
    ld a,h
    or l
    jp z,cliDirectoryDenied
    push hl
    ld hl,(cliOldDirectory)
    ex de,hl
    pop hl
    call cmp_hl_de
    jp z,cliDirectoryDenied
    ld (v_directory),hl
    call fatDirStart
    jp c,cliFileDone
directoryEmptyLoop:
    call fatDirItem
    jp c,directoryEmptyEnd
    ld a,(hl)
    cp '.'
    jp nz,cliDirectoryDenied
    inc hl
    ld a,(hl)
    cp ' '
    jp z,directoryEmptyLoop
    cp '.'
    jp nz,cliDirectoryDenied
    inc hl
    ld a,(hl)
    cp ' '
    jp z,directoryEmptyLoop
    jp cliDirectoryDenied
directoryEmptyEnd:
    or a
    jp nz,cliFileDone
    ld hl,(cliParent)
    ld (v_directory),hl
    ld hl,cliRequest.source
    call fileFindAny
    jp c,cliFileDone
    call cliChanged
    call deleteFileInt
    jp c,cliFileDone
    call saveSectorCache
    call fatFinishWrite
    jp cliFileDone
cliDirectoryDenied:
    ld a,DOS_ERR_ACCESS
    call dosSetError
    jp cliFileDone
cliZero:
    xor a
    ld (hl),a
    inc hl
    dec bc
    ld a,b
    or c
    jp nz,cliZero
    ret

; Новое имя каталога должно быть полным 8.3, без усечения и пустого листа.
; HL сохранён; AF/BC/DE разрушены, CF/A — результат проверки.
directoryValidateLeaf:
    push hl
    ld d,h
    ld e,l
    ld b,MAX_PATH
directoryLeafScan:
    ld a,(hl)
    or a
    jp z,directoryLeafStart
    inc hl
    cp ':'
    jp z,directoryLeafAfterSeparator
    cp '/'
    jp z,directoryLeafAfterSeparator
    cp 05Ch
    jp nz,directoryLeafNext
directoryLeafAfterSeparator:
    ld d,h
    ld e,l
directoryLeafNext:
    dec b
    jp nz,directoryLeafScan
    jp directoryLeafInvalid
directoryLeafStart:
    ex de,hl
    ld b,8
    ld c,0
    ld d,0
directoryLeafChar:
    ld a,(hl)
    or a
    jp z,directoryLeafEnd
    cp '.'
    jp z,directoryLeafExtension
    cp 021h
    jp c,directoryLeafInvalid
    cp '*'
    jp z,directoryLeafInvalid
    cp '?'
    jp z,directoryLeafInvalid
    ld a,b
    or a
    jp z,directoryLeafInvalid
    dec b
    inc c
    inc hl
    jp directoryLeafChar
directoryLeafExtension:
    ld a,d
    or a
    jp nz,directoryLeafInvalid
    ld a,c
    or a
    jp z,directoryLeafInvalid
    ld d,1
    ld b,3
    ld c,0
    inc hl
    jp directoryLeafChar
directoryLeafEnd:
    ld a,c
    or a
    jp z,directoryLeafInvalid
    pop hl
    or a
    ret
directoryLeafInvalid:
    pop hl
    jp blockArgument
