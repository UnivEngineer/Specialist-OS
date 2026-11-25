; Disabled gate is RET: unbound applications pay only CALL/RET, no checks.
; Bind patches this one opcode. The callback may clobber all CPU registers.
    ASSERT $ <= MEM_ROUTE_STATE
    IF $ < MEM_ROUTE_STATE
    DS MEM_ROUTE_STATE-$,0
    ENDIF
    DS 6,0  ; handle, entry, target bank, allocation owner
    ASSERT $ == MEM_QUEUE_HOOK
    DW 0 ; per-bank callback
bankQueueBefore:
    DB 0C9h
    DW MEM_QUEUE_WORK_ENTRY
; Immutable template for a new execution context or an abandoned stack.
bankQueueResetImage:
    DS bankQueueBefore-MEM_ROUTE_STATE,0
    DB 0C9h
