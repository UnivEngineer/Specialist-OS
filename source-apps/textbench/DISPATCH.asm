            include "../../source/include/mxos.inc"
            include "bufferabi.inc"
            org BUFFER_DISPATCH
; До замены SP здесь нет стековых команд: старый стек виден в другой странице.
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
            call z, bufferService
            jp finished
character:  call bios_printChar
finished:   pop hl
            pop de
            pop bc
            ld (savedHL), hl
            ld hl, (savedSP)
            ld sp, hl
            ld hl, (savedHL)
            ld a, (savedPage)
            jp BUFFER_RETURN
; Прямой вариант той же услуги для сравнения с основным ОЗУ.
; C = длина (0..20), все регистры и флаги сохраняются.
bufferService:
            push af
            push bc
            push hl
            ld a, c
            or a
            jp z, bufferDone
            cp BUFFER_SIZE+1
            jp nc, bufferDone
            ld b, c
            ld hl, BUFFER_ADDR
bufferLoop: ld c, (hl)
            call bios_printChar
            inc hl
            dec b
            jp nz, bufferLoop
bufferDone: pop hl
            pop bc
            pop af
            ret
savedHL:    dw 0
savedSP:    dw 0
savedPage:  db 0
            assert $ <= 0FB00h
