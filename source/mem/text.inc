;----------------------------------------------------------------------------
; MXOS
; Печатать NEAR-строку через 32-байтное окно. memPrint выбирает ASCIIZ,
; memPrintTyped берёт режим из memOperation.
; вход: HL — источник, BC — длина для RAW_LIM, PSEUDO и короткой пачки.
; выход: HL на NUL или за массивом, BC/DE сохранены, CF=0.
;----------------------------------------------------------------------------
memPrint:
    ld a,(bankPage)
    or a
    jp z,bios_printStringZ
    xor a
    ld (memOperation),a
memPrintTyped:
    push af
    push bc
    push de
    ld a,(memOperation)
    ld d,a
    cp 2
    jp nc,memPrintLoop
    ld bc,0FFFFh
memPrintLoop:
    ld a,b
    or c
    jp z,memPrintHaveHL
    push bc
    push de
    ld a,b
    or a
    ld b,BANK_TEXT_BYTES
    jp nz,memPrintSize
    ld a,c
    cp b
    jp nc,memPrintSize
    ld b,c
memPrintSize:
    ld a,d
    cp 2
    ld c,0
    jp nc,memPrintReady
    inc c
memPrintReady:
    ld de,BANK_WINDOW
    MEM_TEXT_FILL memPrintFill,memPrintFilled
memPrintFilled:
    ; CF отмечает прочитанный NUL. C — длина печати, B — расход источника.
    ld a,e
    sbc a,BANK_WINDOW & 0FFh
    jp z,memPrintAtNul
    ld c,a
    ld a,e
    sub BANK_WINDOW & 0FFh
    ld b,a
    pop de
    push de
    push hl
    push bc
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
    jp memPrintMain

; MAIN читает текст из окна. Для пачки команд MAIN сохраняет список на стеке.
memPrintMain = MAIN_PRINT_TARGET
    ASSERT $ <= MEM_LOCAL_FORWARD
    IF $ < MEM_LOCAL_FORWARD
    DS MEM_LOCAL_FORWARD-$,0
    ENDIF
    INCLUDE "local-forward.inc"
memOperation:       DB 0
