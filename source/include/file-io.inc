; Local convenience library over the single FILEHANDLE system service.
; fileReadAll/fileWriteAll: HL=FILE_PATH_IO, actual=0 on failure.
; fileReadDescriptor: HL=short-name descriptor, DE=destination; BC/DE/HL saved.
; Paths, requests, handle and buffers are NEAR in the caller's memory.
; Non-reentrant. No private kernel addresses or shared-window manipulation.
; Define FILE_IO_DESCRIPTOR_ONLY to omit whole-path helpers, or
; FILE_IO_HANDLE_ONLY to include only the handle primitives.
    IFNDEF FILE_IO_HANDLE_ONLY
    IFNDEF FILE_IO_DESCRIPTOR_ONLY
fileReadAll:
    xor a
    jp flWhole
fileWriteAll:
    ld a,1
flWhole:
    ld (flWrite),a
    ld (flRequest),hl
    ld de,6
    add hl,de
    ld (hl),0
    inc hl
    ld (hl),0
    call flClear
    ld hl,(flRequest)
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld (flHandle+FH_BUFFER),hl
    ex de,hl
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld (flBuffer),hl
    ex de,hl
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld (flCount),hl
    ex de,hl
    ld hl,(flBuffer)
    push hl
    ld hl,(flCount)
    ld b,h
    ld c,l
    pop hl
    push hl
    add hl,bc
    pop hl
    jp c,flArgument
    push hl
    ld hl,(flRequest)
    ex de,hl
    pop hl
    ld a,FILE_PATH_IO
    call flOverlap
    jp c,flArgument
    ld hl,(flBuffer)
    push hl
    ld hl,(flCount)
    ld b,h
    ld c,l
    pop hl
    ld de,flHandle
    ld a,FILE_HANDLE
    call flOverlap
    jp c,flArgument
    ld a,(flWrite)
    or a
    ld a,FILE_MODE_READ
    jp z,flMode
    ld a,FILE_MODE_WRITE|FILE_MODE_CREATE|FILE_MODE_TRUNCATE
flMode:
    ld (flHandle+FH_MODE),a
    ld a,FILE_OP_OPEN_PATH
    call flCall
    ret c
    ld a,(flWrite)
    or a
    jp nz,flTransfer
    ld hl,(flHandle+FILE_DESCRIPTOR.size+2)
    ld a,h
    or l
    jp nz,flTooLarge
    ld hl,(flHandle+FILE_DESCRIPTOR.size)
    ex de,hl
    ld hl,(flCount)
    ld a,h
    cp d
    jp c,flTooLarge
    jp nz,flReadSize
    ld a,l
    cp e
    jp c,flTooLarge
flReadSize:
    ex de,hl
    ld (flCount),hl
flTransfer:
    ld hl,(flCount)
    ld (flHandle+FH_COUNT),hl
    ld hl,(flBuffer)
    ld (flHandle+FH_BUFFER),hl
    ld a,(flWrite)
    inc a
    call flCall
    call flClose
    ret c
    ld hl,(flRequest)
    ld de,6
    add hl,de
    push hl
    ld hl,(flHandle+FH_COUNT)
    ex de,hl
    pop hl
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    ex de,hl
    ld hl,flHandle
    ld b,FILE_DESCRIPTOR
flDescriptorCopy:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,flDescriptorCopy
    xor a
    ret
flArgument:
    ld a,DOS_ERR_ARGUMENT
    scf
    ret
; Same-bank range intersection, no memory writes. CF=overlap.
flOverlap:
    push af
    ld a,b
    or c
    jp z,flNoOverlap
    push hl
    add hl,bc
    ld a,h
    cp d
    jp c,flBefore
    jp nz,flAfter
    ld a,l
    cp e
    jp c,flBefore
    jp z,flBefore
flAfter:
    pop hl
    pop af
    push hl
    ld h,0
    ld l,a
    add hl,de
    ex de,hl
    pop hl
    ld a,h
    cp d
    ret nz
    ld a,l
    cp e
    ret
flBefore:
    pop hl
flNoOverlap:
    pop af
    or a
    ret
    ENDIF
flTooLarge:
    ld a,DOS_ERR_ARGUMENT
    scf
    jp flClose
    ENDIF
flClose:
    ; Preserve the first I/O error, otherwise report a CLOSE/flush failure.
    push af
    ld a,FILE_OP_CLOSE
    call flCall
    pop bc
    push af
    pop de
    ld a,c
    and 1
    jp nz,flFirstError
    push de
    pop af
    ret
flFirstError:
    push bc
    pop af
    ret
    IFNDEF FILE_IO_HANDLE_ONLY
fileWriteDescriptor:
    ld a,1
    jp flRecord
fileReadDescriptor:
    xor a
flRecord:
    ld (flWrite),a
    push bc
    push de
    push hl
    ex de,hl
    ld (flBuffer),hl
    ex de,hl
    push hl
    call flClear
    pop hl
    push hl
    ld de,FILE_DESCRIPTOR.size
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld (flCount),hl
    ex de,hl
    pop hl
    ld de,flHandle
    ld b,FILE_DESCRIPTOR
flNameCopy:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,flNameCopy
    ld a,DIR_OP_RESOLVED
    MXOS_CALL_REGS MXOS_DOS_DIRECTORY
    ex de,hl
    ld (flHandle+FH_SEEK),hl
    ld de,0
    MXOS_CALL_REGS MXOS_DOS_FILE_GET_SET_DRIVE
    ld (flHandle+FH_DRIVE),a
    ld a,(flWrite)
    or a
    ld a,FILE_MODE_READ
    jp z,flRecordMode
    ld a,FILE_MODE_WRITE|FILE_MODE_CREATE|FILE_MODE_TRUNCATE
flRecordMode:
    ld (flHandle+FH_MODE),a
    ld a,FILE_OP_OPEN_AT
    call flCall
    jp c,flReadReturn
    ld hl,(flHandle+FILE_DESCRIPTOR.size+2)
    ld a,h
    or l
    jp nz,flReadLarge
    ld hl,(flHandle+FILE_DESCRIPTOR.size)
    ld a,(flWrite)
    or a
    jp z,flRecordSize
    ld hl,(flCount)
flRecordSize:
    ld (flHandle+FH_COUNT),hl
    ld hl,(flBuffer)
    ld (flHandle+FH_BUFFER),hl
    ld a,(flWrite)
    inc a
    call flCall
    call flClose
flReadReturn:
    pop hl
    pop de
    pop bc
    ret
flReadLarge:
    call flTooLarge
    jp flReadReturn
    ENDIF
flClear:
    ld hl,flHandle
    ld b,FILE_HANDLE
    xor a
flClearLoop:
    ld (hl),a
    inc hl
    dec b
    jp nz,flClearLoop
    ret
flCall:
    ld hl,flHandle
    MXOS_CALL_REGS MXOS_DOS_FILEHANDLE
    ret
flHandle: DS FILE_HANDLE,0
    IFNDEF FILE_IO_HANDLE_ONLY
    IFNDEF FILE_IO_DESCRIPTOR_ONLY
flRequest: DW 0
    ENDIF
flBuffer:       DW 0
flCount:        DW 0
flWrite:        DB 0
    ENDIF
