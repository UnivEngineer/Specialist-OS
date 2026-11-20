; One private 512-byte coroutine stack. Yield only between complete transfers
; or directory entries. No FAT iterator, buffer data or driver transaction is
; live across a yield. DOS itself always resumes on its original stack.
foControl:
    sub 4
    jp z,foBegin
    dec a
    jp z,foStep
    dec a
    jp z,foAnswer
    dec a
    jp z,foCancel
    dec a
    jp z,foClose
    ; Private snapshot to resident adapter: HL=destination.
    ex de,hl
    ld hl,foJob
    ld bc,FILEOPS_JOB
    call cliCopyBytes
    ld a,(foHasRun)
    or a
    ret z
    ; Progress and confirmation always shows the actual child, not the tree root.
    ld hl,(v_copyRequest)
    ld bc,FILE_COPY_REQUEST
    ld de,KD_OBJECT+FILEOPS_JOB.request
    jp cliCopyBytes
foBegin:
    ld de,foJob
    ld bc,FILEOPS_JOB
    call cliCopyBytes
    ld hl,foJob.request
    ld (v_copyRequest),hl
    ld hl,foJob.state
    ld b,FILEOPS_JOB.files+2-FILEOPS_JOB.state
    xor a
foClear:
    ld (hl),a
    inc hl
    dec b
    jp nz,foClear
    xor a
    ld (foJob.known),a
    ld (fpSkip),a
    ld (fpOpen),a
    ld (foPathText),a
    ld (foPathStamp),a
    ld hl,0
    ld (foJob.totalFiles),hl
    ld (foJob.total),hl
    ld (foJob.total+2),hl
    ld (foJob.required),hl
    ld a,(foJob.mode)
    cp FILEOPS_DELETE
    ld a,FILEOPS_PHASE_PREPARE
    jp nz,foBeginPhase
    ld a,FILEOPS_PHASE_DELETE
foBeginPhase:
    ld (foJob.phase),a
    ld hl,0A55Ah
    ld (foStackGuard),hl
    xor a
    ld (foCancelFlag),a
    ld (foFinished),a
    ld (foHasRun),a
    ld a,1
    ld (foAsync),a
    ld hl,foStart
    ld (foResume),hl
    ld hl,foStackEnd
    ld (foSP),hl
    ret
foAnswer:
    ld a,(foJob.state)
    cp FILEOPS_CONFIRM
    ret nz
    ld a,(KD_OBJECT+FILEOPS_JOB.answer)
    ld (foJob.answer),a
    xor a
    ld (foJob.state),a
    jp foStep
foCancel:
    ld a,(foFinished)
    or a
    ret nz
    ld a,1
    ld (foCancelFlag),a
    xor a
    ld (foJob.state),a
    jp foStep
foClose:
    call foCancel
    ret
foStep:
    call foCheckStack
    ret c
    ld a,(foFinished)
    or a
    ret nz
    ld a,(foJob.state)
    cp FILEOPS_CONFIRM
    ret z
    ld hl,0
    add hl,sp
    ld (foHostSP),hl
    call foContextSave
    ld a,(foHasRun)
    or a
    jp z,foContextReady
    ld a,(foOperationDrive)
    call fileSelectDrive
    jp c,foResumeFailed
    ld hl,(foOperationDirectory)
    ld (v_directory),hl
foContextReady:
    ld hl,(foSP)
    ld sp,hl
    ld hl,(foResume)
    jp (hl)
foResumeFailed:
    ld a,(v_dosError)
    or a
    jp nz,foResumeErrorReady
    ld a,DOS_ERR_IO
foResumeErrorReady:
    ld (foJob.error),a
    ld a,FILEOPS_FAILED
    ld (foJob.state),a
    ld a,1
    ld (foFinished),a
    call foContextRestore
    ret
foStart:
    call foCancelled
    jp c,foFinishedRun
    ld a,1
    ld (foHasRun),a
    ld hl,foFinishedRun
    push hl
    ld hl,foJob.request
    ld (v_copyRequest),hl
    call foRootInfo
    ret c
    ld hl,foJob.request
    ld a,(foJob.mode)
    cp FILEOPS_DELETE
    jp z,foDelete
    or a
    jp z,foPrepareCopy
    ld a,(foJob.request.sourceDrive)
    ld b,a
    ld a,(foJob.request.destinationDrive)
    cp b
    jp z,foMoveSame
foPrepareCopy:
    call foPrepare
    ret c
    call foCancelled
    ret c
    ld hl,foJob.request
    ld (v_copyRequest),hl
    ld a,(foJob.mode)
    call foInternalMode
    ret c
    ld a,(foJob.mode)
    or a
    ret z
    ld a,(copySkipped)
    or a
    ret nz
    ld a,(treeHandled)
    or a
    ret nz
    ld hl,foJob.request
    ld a,1
    jp cliMoveRequestCopy
foMoveSame:
    xor a
    ld (foJob.phase),a
    ld hl,foJob.request
    xor a
    jp cliMoveRequestCopy
foFinishedRun:
    ld a,(v_dosError)
    ld (foJob.error),a
    or a
    ld a,FILEOPS_DONE
    jp z,foSetFinished
    ld a,FILEOPS_FAILED
foSetFinished:
    ld (foJob.state),a
    ld a,1
    ld (foFinished),a
    jp foBack
foCheckpoint:
    ld a,(foAsync)
    or a
    jp z,foPollCancel
    call foCancelled
    ret c
    call foYield
foCancelled:
    ld a,(foCancelFlag)
    or a
    ret z
    ld a,DOS_ERR_CANCELLED
    jp dosSetError
foPollCancel:
    ; The synchronous compatibility path still observes Escape. No DOS call.
    push bc
    push de
    push hl
    MXOS_CALL MXOS_BIOS_KEY_SCAN
    cp 01Bh
    pop hl
    pop de
    pop bc
    jp nz,foPollReady
    ld a,DOS_ERR_CANCELLED
    jp dosSetError
foPollReady:
    or a
    ret
foYield:
    push af
    push bc
    push de
    push hl
    ld a,(v_drive)
    ld (foOperationDrive),a
    ld hl,(v_directory)
    ld (foOperationDirectory),hl
    ld hl,0
    add hl,sp
    ld (foSP),hl
    ld hl,foContinue
    ld (foResume),hl
foBack:
    ld hl,(foHostSP)
    ld sp,hl
    call foCheckStack
    call foContextRestore
    xor a
    ret
foContinue:
    pop hl
    pop de
    pop bc
    pop af
    ret
foProgress:
    ld a,(foAsync)
    or a
    ret z
    ld hl,(foJob.bytes)
    ex de,hl
    ld hl,(v_copyBytes)
    add hl,de
    ld (foJob.bytes),hl
    ret nc
    ld hl,(foJob.bytes+2)
    inc hl
    ld (foJob.bytes+2),hl
    ret
; A skipped file completes its source payload without transferring bytes.
foFileSkipped:
    ld a,(foAsync)
    or a
    ret z
    ld e,FILE_COPY_REQUEST.source+FILE_DESCRIPTOR.size
    call fc_Address
    ex de,hl
    ld hl,foJob.bytes
    ld b,4
    or a
foSkipBytes:
    ld a,(de)
    adc a,(hl)
    ld (hl),a
    inc de
    inc hl
    dec b
    jp nz,foSkipBytes
foFileDone:
    ld a,(foAsync)
    or a
    ret z
    ld hl,(foJob.files)
    inc hl
    ld (foJob.files),hl
    ret
foJob: FILEOPS_JOB
foChunkStarted: DB 0
foHasRun: DB 0
foAsync: DB 0
foCancelFlag: DB 0
foFinished: DB 0
foSP: DW 0
foHostSP: DW 0
foResume: DW 0
foCallerDirectory: DW 0
foOperationDrive: DB 0
foOperationDirectory: DW 0
foCallerDrive: DB 0
foCallerDirectories: DS 16,0
foContextSave:
    ld hl,(v_directory)
    ld (foCallerDirectory),hl
    ld a,(v_drive)
    ld (foCallerDrive),a
    ld hl,v_directories
    ld de,foCallerDirectories
    ld bc,16
    jp cliCopyBytes
foContextRestore:
    ld a,(v_dosError)
    push af
    ld a,(foCallerDrive)
    call fileSelectDrive
    ld hl,foCallerDirectories
    ld de,v_directories
    ld bc,16
    call cliCopyBytes
    ld hl,(foCallerDirectory)
    ld (v_directory),hl
    pop af
    ld (v_dosError),a
    ret
foCheckStack:
    ld hl,(foStackGuard)
    ld a,h
    cp 0A5h
    jp nz,foStackFailed
    ld a,l
    cp 05Ah
    ret z
foStackFailed:
    ld a,DOS_ERR_CORRUPT
    ld (foJob.error),a
    ld a,FILEOPS_FAILED
    ld (foJob.state),a
    ld a,1
    ld (foFinished),a
    scf
    ret
foStackGuard: DW 0A55Ah
foStack: DS 512,0
foStackEnd:
