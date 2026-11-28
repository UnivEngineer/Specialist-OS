;----------------------------------------------------------------------------
; MXOS
; Печатать NEAR-строку через 32-байтное окно. memPrint выбирает ASCIIZ,
; memPrintTyped берёт режим из memOperation.
; вход: HL — источник, BC — длина для LIM, RAW_LIM, PSEUDO и короткой пачки.
; выход: HL на NUL или за массивом, BC/DE сохранены, CF=0.
;----------------------------------------------------------------------------
memPrint:
    ld a,(bankPage)
    or a
    jp z,mon2_printStringOld
    xor a
    ld (memOperation),a
memPrintTyped:
    push bc
    push de
    ld a,(memOperation)
    ld d,a
memPrintLoop:
    ld a,d
    cp MEM_TEXT_RAW_LIM
    jp c,memPrintFrame
    ld a,b
    or c
    jp z,memPrintHaveHL
memPrintFrame:
    push bc
    push hl
    push de
    ld de,BANK_WINDOW
    jp c,memPrintZ
    ld a,b
    or a
    jp nz,memPrintStack
    ld a,c
    cp BANK_TEXT_BYTES
    jp nc,memPrintStack
memPrintBytes:
memPrintStackThreshold:
    cp BANK_TEXT_STACK_MIN
    jp nc,memPrintNear
    cp 4
    jp c,memPrintTail
    rra
    or a
    rra
    ld b,a
    call bankCopyQuads
    ld a,c
    and 3
    jp z,memPrintFilled
memPrintTail:
    ld b,a
    call bankCopyBytes
    jp memPrintFilled
memPrintFilled:
    pop de
    push de
    ld hl,(bankServiceSP)
    push hl
    ld hl,0
    add hl,sp
    ld a,(bankDepth)
    or a
    call nz,bankPublishSP
    ld a,(bankPage)
    dec a
    ld b,a
    ld (BANK_IO_MAIN),a
memPrintJumpMain:
    jp memPrintMain

; MAIN читает текст из окна. Для пачки команд MAIN сохраняет список на стеке.
memPrintMain = MAIN_PRINT_TARGET
    ASSERT $ <= MEM_LOCAL_FORWARD
    IF $ < MEM_LOCAL_FORWARD
    DS MEM_LOCAL_FORWARD-$,0
    ENDIF
    INCLUDE "local-forward.inc"
