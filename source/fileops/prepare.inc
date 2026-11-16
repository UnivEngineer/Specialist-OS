; Preparation is read-only. ENUM owns its cursor; the copy/delete walker is idle.
; treeFrames is reused as destination frames: cluster, free slots, prefix length.
foSkipPrepare:
    ld a,(foJob.phase)
    cp FILEOPS_PHASE_PREPARE
    ret nz
    ld a,1
    ld (fpSkip),a
    ret
foPrepare:
    ld a,(foJob.flags)
    and FILEOPS_NO_PREPARE
    jp nz,fpSkipped
    ld a,(foJob.request.source+FILE_DESCRIPTOR.attrib)
    and 10h
    jp z,fpSingleFile
    ld a,(foJob.flags)
    and FILEOPS_NO_PREPARE
    jp nz,fpSkipped
    ld a,FILEOPS_PHASE_PREPARE
    ld (foJob.phase),a
    call foYield
    call foCancelled
    ret c
    ld a,(foJob.request.destinationDrive)
    call fileChangeDrive
    ret c
    ld hl,(v_diskInfo.clusterBytes)
    ld (fpClusterBytes),hl
    ld hl,treeFrames
    ld (fpFrame),hl
    xor a
    ld (fpDepth),a
    ld hl,(foJob.request.destinationDirectory)
    call fpDirectory
    ret c
    call fpFrameInit
    ld hl,foJob.request
    ld de,treeRequest
    ld bc,FILE_COPY_REQUEST
    call cliCopyBytes
    ld hl,treeRequest
    ld (v_copyRequest),hl
    call fpItem
    ret c
    ld a,(foJob.request.source+FILE_DESCRIPTOR.attrib)
    and 10h
    jp z,fpComplete
    ld hl,foPathText
    ld c,ENUM_RECURSIVE
    xor a
    call enumRun
    ret c
    ld a,1
    ld (fpOpen),a
fpNext:
    call foCheckpoint
    jp c,fpExit
    ld a,(fpSkip)
    or a
    jp nz,fpSkipped
    ld a,ENUM_NEXT
    call enumRun
    jp c,fpEnd
    ld a,h
    or l
    jp z,fpNext
    ld de,treeRequest.source
    ld bc,32
    call cliCopyBytes
    ld de,foPathText
    ld bc,MAX_PATH
    call cliCopyBytes
    ld hl,foPathStamp
    inc (hl)
    ld hl,treeRequest.source
    ld de,treeRequest.destination
    ld bc,11
    call cliCopyBytes
    call fpItem
    jp nc,fpNext
    jp fpExit
fpEnd:
    or a
    jp nz,fpExitError
    call fpClose
fpComplete:
    ld a,(fpSkip)
    or a
    jp nz,fpSkipped
    ld a,(foJob.request.destinationDrive)
    call fileChangeDrive
    ret c
    call fatGetDriveFreeSpace
    ret c
    ld hl,(foJob.required)
    call cmp_hl_de
    jp c,fpValidated
    jp z,fpValidated
    ld a,DOS_ERR_FULL
    jp dosSetError
fpValidated:
    ld a,1
    ld (foJob.known),a
    jp fpReady
fpSkipped:
    call fpClose
    xor a
    ld (foJob.known),a
fpReady:
    xor a
    ld (foJob.phase),a
    ld hl,0
    ld (foJob.files),hl
    ld hl,foJob.request
    ld (v_copyRequest),hl
    call foRootInfo
    ret
fpExitError:
    scf
fpExit:
    push af
    call fpClose
    pop af
    ld (v_dosError),a
    ret
fpClose:
    ld a,(fpOpen)
    or a
    ret z
    xor a
    ld (fpOpen),a
    ld a,ENUM_CLOSE
    jp enumRun

; Select the destination parent frame using the full source path's parent length.
fpItem:
    ld hl,foPathText
    ld b,0
    ld c,0
fpParentLength:
    ld a,(hl)
    or a
    jp z,fpParentReady
    inc hl
    inc b
    cp 05Ch
    jp nz,fpParentLength
    ld c,b
    jp fpParentLength
fpParentReady:
    ld a,b
    ld (fpLength),a
fpParentFrame:
    ld hl,(fpFrame)
    ld de,4
    add hl,de
    ld a,(hl)
    cp c
    jp z,fpParentFound
    jp c,fpParentSet
    ld a,(fpDepth)
    or a
    jp z,dosCorrupt
    dec a
    ld (fpDepth),a
    ld hl,(fpFrame)
    ld de,-8
    add hl,de
    ld (fpFrame),hl
    jp fpParentFrame
fpParentSet:
    ; The only unset prefix is the initial destination-parent frame.
    ld (hl),c
fpParentFound:
    ld a,(foJob.request.destinationDrive)
    call fileChangeDrive
    ret c
    ld hl,(fpFrame)
    call treeReadWord
    ld (v_directory),hl
    inc hl
    ld a,h
    or l
    jp z,fpMissing
    ld hl,treeRequest.destination
    call fileFindAny
    jp nc,fpExisting
    ld a,(v_dosError)
    cp DOS_ERR_NOT_FOUND
    jp nz,fpExitError
fpMissing:
    xor a
    ld (fpExists),a
    call fpSlot
    ret c
    jp fpAccount
fpExisting:
    ld a,1
    ld (fpExists),a
    ld a,(v_tmpFileDescr+FILE_DESCRIPTOR.attrib)
    ld b,a
    ld a,(treeRequest.source+FILE_DESCRIPTOR.attrib)
    and 10h
    xor b
    and 10h
    jp nz,fpTypeBad
fpAccount:
    ld a,(treeRequest.source+FILE_DESCRIPTOR.attrib)
    and 10h
    jp nz,fpSubdirectory
    ld hl,treeRequest.source+FILE_DESCRIPTOR.size
    ld de,foJob.total
    call fpAdd32
    ret c
    ld hl,(foJob.files)
    inc hl
    ld (foJob.files),hl
    ld hl,treeRequest.source+FILE_DESCRIPTOR.size
    call fpClusters
    ret c
    ld (fpWanted),hl
    ld a,(fpExists)
    or a
    jp z,fpFileNew
    ld hl,v_tmpFileDescr+FILE_DESCRIPTOR.size
    call fpClusters
    ret c
    ex de,hl
    ld hl,(fpWanted)
    call sub_hl_de
    jp nc,fpFileNewNeed
    xor a
    ret                      ; never rely on releases from future overwrites
fpFileNewNeed:
    jp fpNeed
fpFileNew:
    ld hl,(fpWanted)
    jp fpNeed
fpTypeBad:
    ld a,DOS_ERR_EXISTS
    jp dosSetError
fpSubdirectory:
    ld a,(fpExists)
    or a
    jp z,fpNewDirectory
    ld hl,(v_tmpFileDescr+FILE_DESCRIPTOR.firstCluster)
    call fpDirectory
    ret c
    jp fpPush
fpNewDirectory:
    ld hl,1
    call fpNeed
    ret c
    ld hl,0FFFFh
    ld (fpDirectoryCluster),hl
    call fpEntriesPerCluster
    dec hl
    dec hl
    ld (fpFree),hl
fpPush:
    ld a,(fpDepth)
    inc a
    cp TREE_DEPTH+1
    jp nc,blockArgument
    ld (fpDepth),a
    ld hl,(fpFrame)
    ld de,8
    add hl,de
    ld (fpFrame),hl
    call fpFrameInit
    ld hl,(fpFrame)
    ld de,4
    add hl,de
    ld a,(fpLength)
    inc a
    ld (hl),a
    xor a
    ret
fpFrameInit:
    ld hl,(fpFrame)
    ex de,hl
    ld hl,(fpDirectoryCluster)
    ex de,hl
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    push hl
    ld hl,(fpFree)
    ex de,hl
    pop hl
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    ld (hl),0
    ret
; Count reusable physical entries, including the tail after the first 00h.
fpDirectory:
    ld (fpDirectoryCluster),hl
    ld (v_directory),hl
    ld hl,0
    ld (fpFree),hl
    xor a
    ld (fpZero),a
    call fatDirStart
    ret c
fpDirectoryCount:
    call fatDirNext
    jp c,fpDirectoryEnd
    ld a,(fpZero)
    or (hl)
    jp z,fpFreeEntry
    ld a,(fpZero)
    or a
    jp nz,fpFreeEntry
    ld a,(hl)
    cp 0E5h
    jp nz,fpDirectoryCount
    jp fpCountEntry
fpFreeEntry:
    ld a,1
    ld (fpZero),a
fpCountEntry:
    ld hl,(fpFree)
    inc hl
    ld (fpFree),hl
    jp fpDirectoryCount
fpDirectoryEnd:
    or a
    ret z
    scf
    ret
fpSlot:
    ld hl,(fpFrame)
    inc hl
    inc hl
    push hl
    call treeReadWord
    ld a,h
    or l
    jp nz,fpSlotFree
    ld hl,(fpFrame)
    call treeReadWord
    ld a,h
    or l
    jp z,fpRootFull
    ld hl,1
    call fpNeed
    jp c,fpSlotError
    call fpEntriesPerCluster
fpSlotFree:
    dec hl
    ex de,hl
    pop hl
    ld (hl),e
    inc hl
    ld (hl),d
    xor a
    ret
fpRootFull:
    ld a,DOS_ERR_DIR_FULL
    call dosSetError
fpSlotError:
    pop hl
    ret
fpEntriesPerCluster:
    ld hl,(fpClusterBytes)
    ld b,5
fpEntryShift:
    or a
    ld a,h
    rra
    ld h,a
    ld a,l
    rra
    ld l,a
    dec b
    jp nz,fpEntryShift
    ret
fpNeed:
    ex de,hl
    ld hl,(foJob.required)
    add hl,de
    jp c,fpFull
    ld (foJob.required),hl
    or a
    ret
fpFull:
    ld a,DOS_ERR_FULL
    jp dosSetError
; ceil(uint32 size / destination clusterBytes), checked 16-bit cluster count.
fpClusters:
    ld de,fpSize
    ld bc,4
    call cliCopyBytes
    ld hl,(fpClusterBytes)
    dec hl
    ex de,hl
    ld hl,(fpSize)
    add hl,de
    ld (fpSize),hl
    jp nc,fpRoundReady
    ld hl,(fpSize+2)
    inc hl
    ld (fpSize+2),hl
    ld a,h
    or l
    jp z,fpFull
fpRoundReady:
    ld hl,(fpClusterBytes)
fpShiftSize:
    ld a,h
    or a
    jp nz,fpShiftSizeGo
    ld a,l
    cp 1
    jp z,fpSizeReady
fpShiftSizeGo:
    or a
    ld a,h
    rra
    ld h,a
    ld a,l
    rra
    ld l,a
    push hl
    ld hl,fpSize+3
    ld b,4
    or a
fpShiftBytes:
    ld a,(hl)
    rra
    ld (hl),a
    dec hl
    dec b
    jp nz,fpShiftBytes
    pop hl
    jp fpShiftSize
fpSizeReady:
    ld hl,(fpSize+2)
    ld a,h
    or l
    jp nz,fpFull
    ld hl,(fpSize)
    ret
; HL=u32 source, DE=u32 destination. Checked; no hidden bank transitions.
fpAdd32:
    ld b,4
    or a
fpAddByte:
    ld a,(de)
    adc a,(hl)
    ld (de),a
    inc de
    inc hl
    dec b
    jp nz,fpAddByte
    ret nc
    jp blockArgument
fpClusterBytes: DW 256
fpDirectoryCluster: DW 0
fpFree: DW 0
fpFrame: DW 0
fpWanted: DW 0
fpSize: DS 4,0
fpDepth: DB 0
fpLength: DB 0
fpExists: DB 0
fpZero: DB 0
fpSkip: DB 0
fpOpen: DB 0

fpSingleFile:
    ld hl,foJob.request.source+FILE_DESCRIPTOR.size
    ld de,foJob.total
    ld bc,4
    call cliCopyBytes
    ld a,1
    ld (foJob.known),a
    xor a
    ld (foJob.phase),a
    ret
