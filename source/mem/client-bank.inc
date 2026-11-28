; Частные обработчики DOS/EXEC. В MAIN в этом диапазоне размещается таблица RAMFOS.

;----------------------------------------------------------------------------
; MXOS
; Передать пакет DOS из банкового профиля MEM.
; вход: A = группа минус 1, B = группа, C = номер службы. DE = MX_REGS в исходном банке.
; выход: AF, BC, DE и HL — результат службы. При отказе диспетчера CF=1, A — код ошибки,
; пакет сохраняется. После выполненного запроса пакет обновляется. SP и банк восстановлены.
;----------------------------------------------------------------------------
bankMxDos:
    dec a
    jp nz,bankMxUnsupported
    push de
    ld a,(bankPage)
    cp 1
    jp z,bankMxLocalDos
    ld a,1
    ld hl,BANK_DOS_GATE
bankMxInvoke:
    call bankCallPacket
    jp bankMxResult
bankMxLocalDos:
    call BANK_DOS_GATE+3
bankMxResult:
    push af
    push bc
    push de
    push hl
    ld a,(BANK_FRAME_STATUS)
    or a
    jp nz,bankMxReturned
    ld hl,8
    add hl,sp
    ld e,(hl)
    inc hl
    ld d,(hl)
    ld hl,0
    add hl,sp
    ld b,MX_REGS/4
    call bankCopyQuads
bankMxReturned:
    pop hl
    pop de
    pop bc
    pop af
    inc sp
    inc sp
    ret

;----------------------------------------------------------------------------
; MXOS
; Выбрать текстовый шлюз для короткой банковой пачки команд BIOS.
; вход: BC = номер операции, DE = MX_REGS в исходном банке. Пакет содержит
; указатель HL на пачку и её размер в BC, исходный DE сохранён на стеке.
; выход: короткая пачка обрабатывается через TEXT; остальные идут обычным
; путём банкового BIOS. AF, BC, DE и HL — результат, SP и банк восстановлены.
;----------------------------------------------------------------------------
memBiosQueue:
    ld hl,4
    add hl,de
    ld a,(hl)
    dec a
    cp BANK_TEXT_BYTES
    jp nc,memBiosGeneric
    inc hl
    ld a,(hl)
    or a
    jp nz,memBiosGeneric
    ld a,MEM_TEXT_BATCH
    jp memBiosText

;----------------------------------------------------------------------------
; MXOS
; Заполнить окно TEXT очередной порцией ASCIIZ из текущего банка.
; вход: HL = адрес строки, DE = начало окна, кадр TEXT на стеке.
; выход: без возврата в memPrintFilled при ненулевом размере C, иначе
; в memPrintZEmpty. HL указывает на очередной байт или NUL, AF и BC меняются.
;----------------------------------------------------------------------------
memPrintZ:
    call bankTextChecked
    ld a,e
    sub BANK_WINDOW & 255
    ld c,a
    jp nz,memPrintFilled
    jp memPrintZEmpty
