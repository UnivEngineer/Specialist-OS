; Public asynchronous job; packets are local to the caller (kdPage).
; Exactly one FILEOPS transaction pins the lazy DOS arena until CLOSE.
fileopsService:
    ld a,(kdRegs+7)
    cp FILEOPS_SKIP_PREPARE+1
    jp nc,kdBad
    ld (fileopsOp),a
    ld hl,(kdRegs)
    ld (kdClientObject),hl
    ld de,KD_OBJECT
    ld a,(fileopsOp)
    or a
    ld bc,5                   ; Handle and answer only; remaining input is BEGIN-only.
    jp nz,fileopsFetch
    ld bc,FILEOPS_JOB
fileopsFetch:
    call kdFetch
    jp c,kdBad
    ld a,(fileopsOp)
    or a
    jp z,fileopsBegin
    ld a,(fileopsPublic)
    or a
    jp z,kdBad
    call memoryOwnerCurrent
    ld hl,fileopsOwner
    cp (hl)
    jp nz,kdBad
    ld hl,(KD_OBJECT+FILEOPS_JOB.handle)
    ex de,hl
    ld hl,(fileopsHandle)
    call cmp_hl_de
    jp nz,kdBad
    ld a,(fileopsOp)
    cp FILEOPS_ANSWER
    jp nz,fileopsControl
    ld a,(KD_OBJECT+FILEOPS_JOB.answer)
    cp 4
    jp nc,kdBad
fileopsControl:
    ld a,(fileopsOp)
    cp FILEOPS_SKIP_PREPARE
    ld a,13
    jp z,fileopsControlCall
    ld a,(fileopsOp)
    add a,4
fileopsControlCall:
    call fileopsInvoke
    jp fileopsPublish
fileopsBegin:
    ld a,0FFh
    ld (fileopsPathStamp),a
    ld hl,(KD_OBJECT+FILEOPS_JOB.path)
    ld a,h
    or l
    ld bc,MAX_PATH
    call nz,kdRange
    jp c,kdBad
    ld a,(KD_OBJECT+FILEOPS_JOB.mode)
    cp FILEOPS_DELETE
    jp z,fileopsBeginValid
    jp nc,kdBad
    ; A packed transfer request must carry an actual destination leaf.
    ld a,(KD_OBJECT+FILEOPS_JOB.request+FILE_COPY_REQUEST.destination)
    cp ' '+1
    jp c,kdBad
fileopsBeginValid:
    call fileopsEnsure
    jp c,fileopsApiError
    ld hl,(fileopsNext)
    ld a,h
    or l
    jp z,kdBad
    ld (fileopsHandle),hl
    ld (KD_OBJECT+FILEOPS_JOB.handle),hl
    inc hl
    ld (fileopsNext),hl
    ld hl,KD_DATA
    ld (KD_OBJECT+FILEOPS_JOB.request+FILE_COPY_REQUEST.buffer),hl
    ld hl,KD_CHUNK
    ld (KD_OBJECT+FILEOPS_JOB.request+FILE_COPY_REQUEST.bufferSize),hl
    ld hl,0
    ld (KD_OBJECT+FILEOPS_JOB.request+FILE_COPY_REQUEST.onExists),hl
    call memoryOwnerCurrent
    ld (fileopsOwner),a
    ld a,1
    ld (fileopsActive),a
    ld (fileopsPublic),a
    ld hl,KD_OBJECT
    ld a,4
    call fileopsInvoke
fileopsPublish:
    ld hl,KD_OBJECT
    ld a,9
    call fileopsInvoke
    ld a,(fileopsOp)
    cp FILEOPS_CLOSE
    jp nz,fileopsStore
    xor a
    ld (fileopsActive),a
    ld (fileopsPublic),a
    ld hl,0
    ld (KD_OBJECT+FILEOPS_JOB.handle),hl
fileopsStore:
    ld hl,(KD_OBJECT+FILEOPS_JOB.path)
    ld a,h
    or l
    jp z,fileopsNoPath
    push hl
    ld a,12
    call fileopsInvoke
    push hl
    ld hl,fileopsPathStamp
    cp (hl)
    ld (hl),a
    pop hl
    pop de
    jp z,fileopsNoPath
    call kdStore
    jp c,kdBad
fileopsNoPath:
    ld hl,0
    ld (KD_OBJECT+FILEOPS_JOB.request+FILE_COPY_REQUEST.buffer),hl
    ld (KD_OBJECT+FILEOPS_JOB.request+FILE_COPY_REQUEST.bufferSize),hl
    ld (KD_OBJECT+FILEOPS_JOB.request+FILE_COPY_REQUEST.onExists),hl
    ld hl,(kdClientObject)
    ex de,hl
    ld hl,KD_OBJECT
    ld bc,FILEOPS_JOB.request ; Common reply is the compact 26-byte header.
    ld a,(KD_OBJECT+FILEOPS_JOB.state)
    cp FILEOPS_CONFIRM
    jp nz,fileopsReply
    ld bc,FILEOPS_JOB         ; The current descriptors are needed for a conflict.
fileopsReply:
    call kdStore
    jp c,kdBad
    xor a
    call kdCapture
    jp kdRestorePointer
fileopsApiError:
    call kdCapture
    jp kdRestorePointer

; C=terminating allocation owner. Cancel before its caller buffers disappear.
fileopsReleaseOwner:
    ld a,(fileopsPublic)
    or a
    ret z
    ld a,(fileopsOwner)
    cp c
    ret nz
    push bc
    ld a,8
    call fileopsInvoke
    xor a
    ld (fileopsPublic),a
    ld (fileopsActive),a
    pop bc
    ret
