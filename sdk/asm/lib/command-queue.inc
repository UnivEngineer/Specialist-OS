; Direct writer for BIOS, DOS and scoped DLL receivers. INCLUDE once in the caller's code.
; Init: HL=buffer, BC=allocated capacity (>=3), DE=normal record reserve (3..min(capacity,255)).
; Fast writer loads (cqNext), writes <=maxCommand bytes, then Commit(HL=end).
; Commit returns the next write pointer in HL, BC/DE preserved, CF=0.
; Reserve(BC=exact bytes) handles a larger command before writing; CF/A error.
; Record: opcode, totalBytes, mask, selected HL/DE/BC/AF words, inline data.
; mask bits 0..3 select words, bit4 sets HL to the following inline data.
; Opcode 0 terminates padding. Queries/GETCH are synchronous, not queued.
commandQueueInit:
    ld a,b
    or a
    jp z,cqInitSmall
    jp cqInitCapacity
cqInitSmall:
    ld a,c
    cp 3
    jp c,cqArgument
cqInitCapacity:
    push hl
    push bc
    push de
    add hl,bc
    jp c,cqInitFailure
    push hl
    ld h,b
    ld l,c
    ld a,d
    or a
    IFNDEF COMMAND_QUEUE_SEND
        jp nz,cqInitFailureEnd
    ELSE
        jp nz,cqInitMaximum
    ENDIF
    ld a,e
    cp 3
    jp c,cqInitFailureEnd
cqInitMaximum:
    call cqCompare
    jp c,cqInitFailureEnd
    ld (cqAllocation),hl
    pop hl
    pop de
    pop bc
    pop hl
    ld (cqBegin),hl
    ld (cqNext),hl
    push hl
    ex de,hl
    ld (cqMaximum),hl
    ld hl,0
    ld (cqUsed),hl
    xor a
    ld (cqError),a
    ld (cqTarget),a
    ld (cqHandle),hl
    call cqSetCapacity
    call mx_memInfo
    ld (cqBank),a
    ld hl,cqSend
    call mx_queueBind
    pop hl
    or a
    ret
cqInitFailureEnd:
    pop hl
cqInitFailure:
    pop de
    pop bc
    pop hl
cqArgument:
    ld a,MEM_ERROR_RANGE
    scf
    ret
; HL-DE unsigned comparison, input pairs preserved.
cqCompare:
    ld a,h
    cp d
    ret nz
    ld a,l
    cp e
    ret
commandQueueReserve:
    ld a,b
    or a
    IFNDEF COMMAND_QUEUE_SEND
        jp z,cqReserveShort
        ld a,(cqTarget)
        cp MX_QUEUE_DLL
        jp nz,cqArgument
        jp cqReserveCapacity
cqReserveShort:
    ELSE
        jp nz,cqReserveCapacity
    ENDIF
    ld a,c
    cp 3
    jp c,cqArgument
cqReserveCapacity:
    push de
    ld d,b
    ld e,c
    ld hl,(cqCapacity)
    call cqCompare
    pop de
    jp c,cqArgument
    push de
    ld hl,(cqNext)
    add hl,bc
    jp c,cqReserveFlush
    ex de,hl
    ld hl,(cqEnd)
    call cqCompare
    jp nc,cqReserveReady
cqReserveFlush:
    call mx_queueFlush
cqReserveReady:
    pop de
    ld hl,(cqNext)
    or a
    ret
commandQueueCommit:
    push bc
    push de
    ld (cqNext),hl
    ex de,hl
    ld hl,(cqBegin)
    ld a,e
    sub l
    ld l,a
    ld a,d
    sbc a,h
    ld h,a
    ld (cqUsed),hl
    ld hl,(cqLimit)
    call cqCompare
    call c,mx_queueFlush
    ld hl,(cqNext)
    pop de
    pop bc
    or a
    ret
; A=MX_QUEUE_BIOS/DOS/DLL, HL=DLL handle (ignored for BIOS/DOS).
; Switching drains the old receiver first. Consume cqError before retrying.
commandQueueSelect:
    cp MX_QUEUE_DLL+1
    jp nc,cqArgument
    cp MX_QUEUE_DLL
    jp z,cqSelectDll
    ld hl,0
    jp cqSelectCompare
cqSelectDll:
    push af
    ld a,h
    or l
    jp z,cqSelectInvalid
    pop af
cqSelectCompare:
    push af
    ld a,(cqTarget)
    ld b,a
    pop af
    cp b
    jp nz,cqSelectChange
    ex de,hl
    ld hl,(cqHandle)
    call cqCompare
    ex de,hl
    jp z,cqStatus
    ld a,b
cqSelectChange:
    call mx_queueFlush
    ld (cqTarget),a
    ld (cqHandle),hl
    call cqSetCapacity
cqStatus:
    ld a,(cqError)
    or a
    ret z
    scf
    ret
cqSelectInvalid:
    pop af
    jp cqArgument
cqSetCapacity:
    ld hl,(cqAllocation)
    IFNDEF COMMAND_QUEUE_SEND
        ld a,(cqTarget)
        cp MX_QUEUE_DLL
        jp z,cqCapacityReady
        ld a,h
        or a
        jp z,cqCapacityReady
        ld hl,256
cqCapacityReady:
    ENDIF
    ld (cqCapacity),hl
    ex de,hl
    ld hl,(cqBegin)
    add hl,de
    ld (cqEnd),hl
    ex de,hl
    ld hl,(cqMaximum)
    ld a,e
    sub l
    ld l,a
    ld a,d
    sbc a,h
    ld h,a
    ld (cqLimit),hl
    ret
commandQueueFlush:
    jp mx_queueFlush
cqSend:
    push af
    push bc
    push de
    push hl
    ld hl,(cqUsed)
    ld a,h
    or l
    jp z,cqFlushed
    ld (cqTransfer),hl
    IFNDEF COMMAND_QUEUE_SEND
    ld a,(cqTarget)
    cp MX_QUEUE_DLL
    jp z,cqTransferReady
    ; Round larger copies to CPM32 blocks, within the owned buffer only.
    ; A zero opcode terminates padding. Tiny remainders stay exact.
    ld a,h
    or a
    jp nz,cqTransferReady
    ld a,l
    cp 21
    jp c,cqTransferReady
    and 31
    cp 8
    jp c,cqTransferReady
    ld de,31
    add hl,de
    ld a,l
    and 0E0h
    ld l,a
    ex de,hl
    ld hl,(cqCapacity)
    ld a,h
    cp d
    jp c,cqTransferReady
    jp nz,cqPadded
    ld a,l
    cp e
    jp c,cqTransferReady
cqPadded:
    ld hl,(cqNext)
    ld (hl),0
    ex de,hl
    ld (cqTransfer),hl
    ENDIF
cqTransferReady:
    ld hl,(cqTransfer)
    ld b,h
    ld c,l
    ld hl,(cqBegin)
    ld (cqNext),hl
    ld a,(cqBank)
    ; mx_queueFlush disables recursion. Direct entry is for the hook only.
    IFDEF COMMAND_QUEUE_SEND
        call COMMAND_QUEUE_SEND ; A=source bank, HL=buffer, BC=physical bytes
    ELSE
        call cqDeliver
        jp nc,cqSent
        ld (cqError),a
cqSent:
    ENDIF
    ld hl,0
    ld (cqUsed),hl
cqFlushed:
    pop hl
    pop de
    pop bc
    pop af
    ret
commandQueueClose:
    call mx_queueFlush
    ld hl,0
    jp mx_queueBind
    IFNDEF COMMAND_QUEUE_SEND
cqDeliver:
    ld a,(cqTarget)
    cp MX_QUEUE_DLL
    jp z,cqDeliverDll
    or a
    ld a,(cqBank)
    jp nz,cqDeliverDos
    MXOS_CALL_REGS MXOS_BIOS_COMMAND_BATCH
    ret
cqDeliverDos:
    MXOS_CALL_REGS MXOS_DOS_COMMAND_BATCH
    ret
cqDeliverDll:
    ld (cqPacket),hl
    ld h,b
    ld l,c
    ld (cqPacket+4),hl
    ld a,(cqBank)
    ld (cqPacket+7),a
    ld hl,(cqHandle)
    ld de,cqPacket
    jp mx_routeCall
cqPacket: DW 0,0,0,0
    ENDIF
cqError:        DB 0
cqTarget:       DB 0
cqHandle:       DW 0
cqTransfer:     DW 0
cqAllocation:   DW 0
cqMaximum:      DW 0
cqCapacity:     DW 0
cqLimit:        DW 0
cqBegin:        DW 0
cqNext:         DW 0
cqEnd:          DW 0
cqUsed:         DW 0
cqBank:         DB 0
