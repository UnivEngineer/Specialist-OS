; Disabled gate is RET: unbound applications pay only CALL/RET, no checks.
; Bind patches this one opcode. The callback may clobber all CPU registers.
    ASSERT $ <= MEM_ROUTE_STATE
    DS MEM_ROUTE_STATE-$,0
    DS 6,0  ; handle, entry, target bank, allocation owner
    ASSERT $ == MEM_QUEUE_HOOK
    DW 0 ; per-bank callback
bankQueueBefore:
    DB 0C9h
    DW bankQueueWork
bankQueueWork:
    push af
    push hl
    ld a,0C9h
    ld (bankQueueBefore),a
    push bc
    push de
    ld de,bankQueueReturned
    push de
    ld hl,(MEM_QUEUE_HOOK)
    jp (hl)
bankQueueReturned:
    pop de
    pop bc
    ld a,0C3h
    ld (bankQueueBefore),a
    pop hl
    pop af
    ret
; Immutable template for a new execution context or an abandoned stack.
bankQueueResetImage:
    DS bankQueueBefore-MEM_ROUTE_STATE,0
    DB 0C9h
