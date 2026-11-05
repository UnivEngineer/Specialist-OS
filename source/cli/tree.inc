; COPY дерева и MOVE между томами. Вход из fileCopy после проверки буфера
; и чтения исходного дескриптора; прежний диск лежит в стеке.
; Системный стек не растёт с глубиной: кадры содержат кластеры и индекс.
; Данные файлов идут через явный буфер исходного FILE_COPY_REQUEST.
TREE_DEPTH EQU MAX_PATH/2
TREE_FRAME EQU 8
TREE_CLI_SAVE EQU cliOldDirectory+2-cliSavedDirectories
treeCopyStart:
    pop af
    ld (treeOldDrive),a
    ld a,1
    ld (treeHandled),a
    ld hl,cliSavedDirectories
    ld de,treeCliSave
    ld bc,TREE_CLI_SAVE
    call cliCopyBytes
    ld hl,(v_copyRequest)
    ld de,treeRoot
    ld bc,FILE_COPY_REQUEST
    call cliCopyBytes
    ld hl,treeRoot
    ld de,treeRequest
    ld bc,FILE_COPY_REQUEST
    call cliCopyBytes
    ld a,(v_copyCapacity)
    ld (treeCapacity),a
    xor a
    ld (treeDepth),a
    ld hl,treeFrames
    ld (treeFrame),hl
    ld a,(treeMoveMode)
    or a
    jp z,treeCheckDestination
    ld a,(treeRoot.source+FILE_DESCRIPTOR.attrib)
    and 5
    jp nz,treeDenied
treeCheckDestination:
    ld a,(treeRoot.sourceDrive)
    ld b,a
    ld a,(treeRoot.destinationDrive)
    cp b
    jp nz,treeCreateNode
    ; Та же проверка предков, что при переносе записи внутри тома.
    ld hl,treeRoot
    ld de,cliRequest
    ld bc,FILE_COPY_REQUEST
    call cliCopyBytes
    call cliMoveCheckParent
    jp c,treeExit
treeCreateNode:
    call treeMakeDirectory
    jp c,treeExit
    ; cliNewCluster содержит только что созданный каталог назначения.
    ld hl,(treeRequest.source+FILE_DESCRIPTOR.firstCluster)
    ex de,hl
    ld hl,(treeFrame)
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    push hl
    ld hl,(cliNewCluster)
    ex de,hl
    pop hl
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    xor a
    ld (hl),a
    inc hl
    ld (hl),a
    inc hl
    ld (hl),a
treeNext:
    ld a,(treeRoot.sourceDrive)
    call fileChangeDrive
    jp c,treeExit
    ld hl,(treeFrame)
    call treeReadWord
    ld (v_directory),hl
    call fatDirStart
    jp c,treeExit
    ld hl,(treeFrame)
    ld de,4
    add hl,de
    call treeReadWord
    ex de,hl
treeSkip:
    call fatDirItem
    jp c,treeDirectoryEnd
    ld a,d
    or e
    jp z,treeItem
    dec de
    jp treeSkip
treeItem:
    ld de,treeRequest.source
    ld bc,FILE_DESCR_SIZE
    call cliCopyBytes
    ld a,(treeRequest.source)
    cp 5
    jp nz,treeDecoded
    ld a,0E5h
    ld (treeRequest.source),a
treeDecoded:
    call treeAdvance
    ld a,(treeRequest.source)
    cp '.'
    jp z,treeNext
    ld hl,(treeFrame)
    call treeReadWord
    ld (treeRequest.sourceDirectory),hl
    ld hl,(treeFrame)
    inc hl
    inc hl
    call treeReadWord
    ld (treeRequest.destinationDirectory),hl
    ld hl,treeRequest.source
    ld de,treeRequest.destination
    ld bc,11
    call cliCopyBytes
    ld a,(treeMoveMode)
    or a
    jp z,treeItemAllowed
    ld a,(treeRequest.source+FILE_DESCRIPTOR.attrib)
    and 5
    jp nz,treeDenied
treeItemAllowed:
    ld a,(treeRequest.source+FILE_DESCRIPTOR.attrib)
    and 010h
    jp nz,treeDescend
    ld hl,treeRequest
    ld (v_copyRequest),hl
    ld a,(treeCapacity)
    ld (v_copyCapacity),a
    call fc_FileRangeReady
    jp c,treeExit
    ld a,(copySkipped)
    or a
    jp nz,treeRetained
    ld a,(treeMoveMode)
    or a
    jp z,treeNext
    ld a,(treeRoot.sourceDrive)
    call fileChangeDrive
    jp c,treeExit
    ld hl,(treeRequest.sourceDirectory)
    ld (v_directory),hl
    ld hl,treeRequest.source
    call cliDeletePrepared
    jp c,treeExit
    call treeBackIndex
    jp treeNext
treeDescend:
    ld a,(treeDepth)
    inc a
    cp TREE_DEPTH
    jp nc,treeBadDepth
    ld (treeDepth),a
    ld hl,(treeRequest.source+FILE_DESCRIPTOR.firstCluster)
    ex de,hl
    ld hl,treeFrames
    ld b,a
treeAncestor:
    push hl
    ld c,(hl)
    inc hl
    ld a,(hl)
    cp d
    jp nz,treeAncestorNext
    ld a,c
    cp e
    jp z,treeCycle
treeAncestorNext:
    pop hl
    ld a,l
    add a,TREE_FRAME
    ld l,a
    jp nc,treeAncestorCarry
    inc h
treeAncestorCarry:
    dec b
    jp nz,treeAncestor
    ld (treeFrame),hl
    jp treeCreateNode
treeCycle:
    pop hl
    call dosCorrupt
    jp treeExit
treeDirectoryEnd:
    or a
    jp nz,treeExit
    ld a,(treeMoveMode)
    or a
    jp z,treePop
    ld hl,(treeFrame)
    ld de,6
    add hl,de
    ld a,(hl)
    or a
    jp nz,treePopRetained
    ; Каталог опустел: найти его запись в родителе по кластеру и удалить.
    ld hl,(treeFrame)
    call treeReadWord
    ld (treeChild),hl
    ld a,(treeDepth)
    or a
    ld hl,(treeRoot.sourceDirectory)
    jp z,treeRemoveParent
    ld hl,(treeFrame)
    ld de,-TREE_FRAME
    add hl,de
    call treeReadWord
treeRemoveParent:
    ld (v_directory),hl
    call treeFindChild
    jp c,treeExit
    call cliChanged
    call deleteFileInt
    jp c,treeExit
    call saveSectorCache
    jp c,treeExit
    call fatFinishWrite
    jp c,treeExit
treePop:
    ld a,(treeDepth)
    or a
    jp z,treeSuccess
    dec a
    ld (treeDepth),a
    ld hl,(treeFrame)
    ld de,-TREE_FRAME
    add hl,de
    ld (treeFrame),hl
    ld a,(treeMoveMode)
    or a
    call nz,treeBackIndex
    jp treeNext
treeRetained:
    ld hl,(treeFrame)
    ld de,6
    add hl,de
    ld (hl),1
    jp treeNext
treePopRetained:
    ld a,(treeDepth)
    or a
    jp z,treeSuccess
    dec a
    ld (treeDepth),a
    ld hl,(treeFrame)
    ld de,-TREE_FRAME
    add hl,de
    ld (treeFrame),hl
    jp treeRetained
treeDenied:
    ld a,DOS_ERR_ACCESS
    call dosSetError
    jp treeExit
treeBadDepth:
    call blockArgument
    jp treeExit
treeSuccess:
    xor a
    ld (v_dosError),a
treeExit:
    ld a,(v_dosError)
    push af
    ld hl,treeCliSave
    ld de,cliSavedDirectories
    ld bc,TREE_CLI_SAVE
    call cliCopyBytes
    ld hl,treeRoot
    ld de,cliRequest
    ld bc,FILE_COPY_REQUEST
    call cliCopyBytes
    ld a,(treeOldDrive)
    call fileChangeDrive
    pop bc
    jp cliRestoreStatus
; Создать каталог из компактного имени; общая инициализация . и .. не дублируется.
treeMakeDirectory:
    ld a,(treeRoot.destinationDrive)
    call fileChangeDrive
    ret c
    ld hl,(treeRequest.destinationDirectory)
    ld (v_directory),hl
    ld hl,treeRequest.destination
    call fileFindAny
    jp c,treeMakeNew
    ld a,(v_tmpFileDescr+FILE_DESCRIPTOR.attrib)
    and 010h
    jp z,cliMoveExists
    ld hl,(v_tmpFileDescr+FILE_DESCRIPTOR.firstCluster)
    ld (cliNewCluster),hl
    xor a
    ld (v_dosError),a
    ret
treeMakeNew:
    ld a,(v_dosError)
    cp DOS_ERR_NOT_FOUND
    jp nz,cliMoveError
    ld hl,treeRequest.destination
    ld de,cliRequest.source
    ld bc,11
    call cliCopyBytes
    ex de,hl
    ld bc,FILE_DESCR_SIZE-11
    call cliZero
    ld hl,v_directories
    ld de,cliSavedDirectories
    ld bc,16
    call cliCopyBytes
    ld a,(v_drive)
    ld (cliOldDrive),a
    call directoryCreatePrepared
    ret c
    ld hl,(treeRequest.destinationDirectory)
    ld (v_directory),hl
    ld hl,treeRequest.destination
    call fileFindAny
    ret c
    call markDirSectorAsModified
    ret c
    ld de,11
    add hl,de
    ex de,hl
    ld hl,treeRequest.source+11
    ld bc,15
    call cliCopyBytes
    call saveSectorCache
    ret c
    jp fatFinishWrite
; Курсор перечисления пересоздаётся после файловых операций с общим кэшем.
treeAdvance:
    ld hl,(treeFrame)
    ld de,4
    add hl,de
    inc (hl)
    ret nz
    inc hl
    inc (hl)
    ret
treeBackIndex:
    ld hl,(treeFrame)
    ld de,4
    add hl,de
    ld a,(hl)
    dec (hl)
    or a
    ret nz
    inc hl
    dec (hl)
    ret
treeReadWord:
    ld a,(hl)
    inc hl
    ld h,(hl)
    ld l,a
    ret
treeFindChild:
    call fatDirStart
    ret c
treeFindLoop:
    call fatDirItem
    jp c,dosCorrupt
    ld a,(hl)
    cp '.'
    jp z,treeFindLoop
    push hl
    ld de,FILE_DESCRIPTOR.firstCluster
    add hl,de
    call treeReadWord
    ex de,hl
    ld hl,(treeChild)
    call cmp_hl_de
    pop hl
    jp nz,treeFindLoop
    ld de,treeScratch
    ld bc,11
    call cliCopyBytes
    ld hl,treeScratch
    ld a,(hl)
    cp 5
    jp nz,fileFindAny
    ld (hl),0E5h
    jp fileFindAny
treeMoveMode: DB 0
treeHandled: DB 0
treeOldDrive: DB 0
treeCapacity: DB 0
treeDepth: DB 0
treeFrame: DW 0
treeChild: DW 0
treeRoot: FILE_COPY_REQUEST
treeRequest: FILE_COPY_REQUEST
treeCliSave: DS TREE_CLI_SAVE,0
treeScratch: DS 11,0
treeFrames: DS TREE_FRAME*TREE_DEPTH,0
