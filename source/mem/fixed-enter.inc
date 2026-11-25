;----------------------------------------------------------------------------
; MXOS
; Войти в BIOS по известному адресу из MAIN, DOS или EXEC.
; вход: DE — вектор BIOS, исходный DE сохранён точкой входа; A/C/HL — аргументы.
; выход: AF/BC/DE/SP и логическая страница сохранены, HL — результат BIOS.
;----------------------------------------------------------------------------
memFixedSetTop:
    push af
    or a
    jp memFixedTopMap
memFixedEnter:
    call bankQueueBefore
    push bc
    push af
    ld a,(bankPage)
    dec a
    jp m,memFixedNative
    ld b,a
    pop af
    push af
    ld (BANK_IO_MAIN),a
    jp memFixedMain
memFixedNative:
    pop af
    pop bc
    ex de,hl
    ex (sp),hl
    ex de,hl
    ret
