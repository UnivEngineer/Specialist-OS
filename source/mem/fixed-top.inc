; GET/SET_MEM_TOP работают с локальным стеком вызывающего банка.
; CF выбирает чтение, знак DEC A сообщает, был ли исходный банк MAIN.
; Очередь сохраняет флаги, LD и выбор банка не меняют CF.
memFixedGetTop:
    push af
    scf
memFixedTopMap:
    call bankQueueBefore
    ld a,(bankPage)
    dec a
    jp m,MEM_TOP_MAPPED
    ld (BANK_IO_MAIN),a
    jp MEM_TOP_MAPPED
