; Наблюдатель CLI принадлежит зарегистрировавшей его странице.
; На входе kdRegs: A=CLI_OP_OBSERVER, HL=обработчик или 0; kdPage=клиент.
; На выходе обычный результат cliSetObserver, исходный HL сохранён.
koRegister:
    ld hl,(kdRegs)
    ld bc,1
    call kdRange
    jp c,kdBad
    ld (koAddress),hl
    ld a,(kdPage)
    ld (koPage),a
    ld a,h
    or l
    ld hl,0
    jp z,koInstall
    ld hl,koNotify
koInstall:
    ld (kdRegs),hl
    call kdInvoke
    call kdCapture
    ld hl,(koAddress)
    ld (kdRegs),hl
    jp kdFinish

; Вход A=диск, DE=каталог, C=область изменения; данных по указателям нет.
; Уведомление синхронное. Все регистры сохраняет внешний cliChanged.
; DOS занята текущей операцией: повторный файловый вызов отвергается.
; Пакет отдельный, чтобы не затереть запрос, породивший уведомление.
koNotify:
    ld (koRegs),hl
    ex de,hl
    ld (koRegs+2),hl
    ld h,b
    ld l,c
    ld (koRegs+4),hl
    push af
    pop hl
    ld (koRegs+6),hl
    ld hl,(koAddress)
    ld b,h
    ld c,l
    ld a,(koPage)
    ld hl,bankCallback
    ld de,koRegs
    jp bankCallPacket
koAddress: DW 0
koPage: DB 0
koRegs: DS MX_REGS,0
