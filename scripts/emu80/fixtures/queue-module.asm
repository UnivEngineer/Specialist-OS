; Leaf module for benchmark-command-queue.py. Scalar calls return HL+1.
    INCLUDE "../../../sdk/asm/mxos.inc"
    ORG 06000h
    ld d,a                  ; submitting bank for a DLL batch
    ld a,b
    or c
    jp nz,batch
    inc hl
    ret
batch:
    push hl
    ld hl,0
    add hl,sp
    ld (moduleSP),hl
    pop hl
    push hl
    ld h,b
    ld l,c
    ld (receivedBytes),hl
    pop hl
    ld b,d
    ld a,MEM_OP_GET8
    call mx_memory
    ld a,e
    ld (firstByte),a
    ; Allocate on behalf of this DLL through a nested DOS call.
    ld b,MEM_ALLOC_ANY
    ld hl,17
    ld a,MEM_OP_ALLOC
    MXOS_CALL_REGS MXOS_DOS_MEMORY
    ret
receivedBytes:  DW 0
firstByte:      DB 0
moduleSP:       DW 0
