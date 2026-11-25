; Отдельная реплика теста. В железе нужна одна копия по тому же адресу в MAIN и EXEC.
; Вход: EXEC, AF/BC/DE произвольны, стек инициатора активен.
; Выход: HL — положение курсора MAIN. AF/BC/DE/SP и страница восстановлены.
; Предусловия: валидная страница 2..16, доступный стек MAIN bankServiceSP.
cursorReplica:
    push de
    push af
    ld hl,0
    add hl,sp
    ex de,hl
    ld a,(bankPage)
    dec a
    ld (BANK_IO_MAIN),a
    ld hl,(bankServiceSP)
    ld sp,hl
    call bios_getCursorPos
    ld (BANK_IO_RAM),a
    ex de,hl
    ld sp,hl
    ex de,hl
    pop af
    pop de
    ret

; Только операция чтения позиции, без вызова C81Eh и стека MAIN.
cursorReplicaRead:
    push af
    ld a,(bankPage)
    dec a
    ld (BANK_IO_MAIN),a
    ld hl,(cursorAddress)
    ld (BANK_IO_RAM),a
    pop af
    ret

; Операнд LD A задаётся отдельно в каждой копии страницы при установке.
cursorReplicaReadBound:
    push af
    ld a,0
    ld (BANK_IO_MAIN),a
    ld hl,(cursorAddress)
    ld (BANK_IO_RAM),a
    pop af
    ret
