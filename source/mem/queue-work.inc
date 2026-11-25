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
