            org FONT8_DISPATCH
; Общий шлюз сохраняет AF до переключения. A = страница возврата.
; B = функция: 0 — символ, 1 — буфер, 2 — вспомогательная услуга.
; Контракты услуг задаёт включающая сборка (FONT8 или SHIFT).
; 3 и остальные — пустой контрольный вызов. Все регистры сохранены.
dispatch:   ld (savedHL), hl
            ld (savedPage), a
            ld hl, 0
            add hl, sp
            ld (savedSP), hl
            ld sp, STACK_ADDR
            push bc
            push de
            ld hl, (savedHL)
            push hl
            ld a, b
            or a
            jp z, character
            dec a
            jp z, string
            dec a
            call z, FONT8_SCROLL
            jp finished
character:  call FONT8_RENDER
            jp finished
string:     call FONT8_BUFFER
finished:   pop hl
            pop de
            pop bc
            ld (savedHL), hl
            ld hl, (savedSP)
            ld sp, hl
            ld hl, (savedHL)
            ld a, (savedPage)
            jp BUFFER_RETURN
savedHL:    dw 0
savedSP:    dw 0
savedPage:  db 0
            assert $ <= 0FB00h
