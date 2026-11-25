; MXOS. Вход фиксированного экранного вызова после выбора MAIN.
; HL содержит адрес BIOS-вектора, DE хранит исходный HL, SP ещё принадлежит клиенту.
; Результат и стек возвращает реплицированный хвост MEM в странице инициатора.
biosMemFixedMain:
    ex de,hl                    ; DE держит исходный HL до установки стека.
    ld (memFixedVector+1),hl
    ld hl,0
    add hl,sp
    ld (memFixedCallerSP+1),hl
    ld hl,(bankServiceSP)
    ld sp,hl
    ld hl,bankDepth
    inc (hl)
memFixedCallerSP:
    ld hl,0
    push hl
    ld hl,memFixedReturn
    push hl
    call bankPublishSP
    ex de,hl
memFixedVector:
    jp 0                        ; Операнд — выбранный вектор C8xxh.
; До входа в BIOS оба временных операнда уже использованы. При вложении
; каждый адрес возврата и каждый SP лежат в своём кадре стека MAIN.
memFixedReturn:
    ex de,hl
    ld hl,bankDepth
    dec (hl)
    ld a,b
    pop bc
    call bankPublishSP
    ld h,b
    ld l,c
    jp MEM_SCREEN_RETURN
;----------------------------------------------------------------------------
; MXOS
; Напечатать данные непосредственно из физического окна или выполнить пачку.
; вход: MAIN выбран, HL = стек клиента, B = обратный селектор, C = длина 1..32,
; D = режим TEXT. Печатный движок сохраняет окно до возврата.
; выход: DE = стек клиента, C = число обработанных байтов, стек MAIN освобождён.
;----------------------------------------------------------------------------
biosMemPrintMain:
    ld a,d
    ex de,hl
    ld hl,(bankServiceSP)
    ld sp,hl
    push de
    push bc
    ld d,a
    ld b,0
    ld hl,BANK_WINDOW
    cp 7
    jp z,memPrintBatch
    call bankText.printWindow
    pop bc
    pop de
    ld a,b
    jp MEM_TEXT_RETURN

; Пачка допускает банковый вызов из отдельной команды. На локальном стеке
; остаётся весь список, включая ещё не выполненные записи.
memPrintBatch:
    ld hl,-BANK_TEXT_BYTES
    add hl,sp
    ld sp,hl
    ex de,hl
    ld hl,BANK_WINDOW
    push bc
    call moveMemForward
    pop bc
    ld hl,bankDepth
    inc (hl)
    call bankPublishSP
    call BIOS_BANK_GATE_ADDR+9
    ld hl,BANK_TEXT_BYTES
    add hl,sp
    ld sp,hl
    pop bc
    pop de
    ld hl,bankDepth
    dec (hl)
    call bankPublishSP
    ld a,b
    jp MEM_TEXT_RETURN
