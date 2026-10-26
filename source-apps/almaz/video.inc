; Восстановлено из assets/games/Almaz.rks; без внешних бинарных включений.

; Алмаз: точная реконструкция Almaz.rks. Адреса и исходные байты сохранены.

game_entry:
    jp      game_main ; 1EFDh

; Плитка 16*16. Вход: A=тип, B=X, C=Y. Выход: рисунок на экране.
; DE, HL сохранены; AF, BC и флаги разрушены. Тип 10h использует рисунок 0Ch.

tile_draw:
    push    de ; 1F00h
    push    hl ; 1F01h
    ld      d,005h ; 1F02h
    ld      h,000h ; 1F04h
    cp      010h ; 1F06h
    jp      nz,tile_offset_loop ; 1F08h
    ld      a,00Ch ; 1F0Bh

tile_offset_loop:
    or      a ; 1F0Dh
    rla      ; 1F0Eh
    ld      l,a ; 1F0Fh
    ld      a,h ; 1F10h
    rla      ; 1F11h
    ld      h,a ; 1F12h
    ld      a,l ; 1F13h
    dec     d ; 1F14h
    jp      nz,tile_offset_loop ; 1F15h
    ld      de,t_tiles ; 1F18h
    add     hl,de ; 1F1Bh
    ld      de,00210h ; 1F1Ch
    call    sprite_blit ; 1F1Fh
    pop     hl ; 1F22h
    pop     de ; 1F23h
    ret      ; 1F24h

; Копирование спрайта. Вход: HL=данные, B=X, C=Y, D=ширина/2, E=высота.
; Выход: экран; AF, BC, DE, HL и флаги разрушены. Экран столбцами по 256 байт.

sprite_blit:
    ld      a,b ; 1F25h
    rlca     ; 1F26h
    add     a,090h ; 1F27h
    ld      b,a ; 1F29h
    ld      a,c ; 1F2Ah
    rlca     ; 1F2Bh
    rlca     ; 1F2Ch
    rlca     ; 1F2Dh
    rlca     ; 1F2Eh
    nop      ; 1F2Fh
    ld      c,a ; 1F30h
    push    de ; 1F31h
    push    bc ; 1F32h

sprite_blit_row:
    ld      a,(hl) ; 1F33h
    ld      (bc),a ; 1F34h
    inc     hl ; 1F35h
    inc     bc ; 1F36h
    dec     e ; 1F37h
    jp      nz,sprite_blit_row ; 1F38h
    pop     bc ; 1F3Bh
    inc     b ; 1F3Ch
    pop     de ; 1F3Dh
    dec     d ; 1F3Eh
    ret     z ; 1F3Fh
    push    de ; 1F40h
    push    bc ; 1F41h
    jp      sprite_blit_row ; 1F42h
    db      02Ch,004h,001h,000h,090h,000h,090h,020h,02Ch,004h,001h,000h,090h,000h,090h,040h ; 1F45h
    db      02Ch,004h,001h,000h,090h,000h,090h,060h,02Ch,004h,001h,000h,090h,000h,090h,080h ; 1F55h
    db      02Ch,004h,001h,000h,090h,000h,090h,0A0h,02Ch,004h,001h,000h,090h,000h,090h,0C0h ; 1F65h
    db      02Ch,004h,001h,000h,090h,000h,090h,0E0h,02Ch,004h,001h ; 1F75h

; Взрыв 48*48 вокруг клетки. Вход: B=X, C=Y, A=фаза 0/1.
; DE, HL сохранены; AF, BC и флаги разрушены.

explosion_draw:
    push    de ; 1F80h
    push    hl ; 1F81h
    dec     b ; 1F82h
    dec     c ; 1F83h
    ld      hl,t_explosion_first ; 1F84h
    or      a ; 1F87h
    jp      z,explosion_blit ; 1F88h
    inc     h ; 1F8Bh
    inc     h ; 1F8Ch

explosion_blit:
    ld      de,00630h ; 1F8Dh
    call    sprite_blit ; 1F90h
    pop     hl ; 1F93h
    pop     de ; 1F94h
    ret      ; 1F95h
    db      004h,004h,000h,090h,000h,090h,000h,090h,004h,004h,000h,090h,000h,090h,000h,090h ; 1F96h
    db      004h,004h,000h,090h,000h,090h,000h,090h,004h,004h,000h,090h,000h,090h,000h,090h ; 1FA6h
    db      004h,004h,000h,090h,000h,090h,000h,090h,004h,004h,000h,090h,000h,090h,000h,090h ; 1FB6h
    db      004h,004h,000h,090h,000h,090h,000h,090h,004h,004h,000h,090h,000h,090h,000h,090h ; 1FC6h
    db      004h,004h,000h,090h,000h,090h,000h,090h,004h,004h,000h,090h,000h,090h,000h,090h ; 1FD6h
    db      004h,004h,000h,090h,000h,090h,000h,090h,004h,004h,03Eh,0FFh,032h,0DFh,029h,000h ; 1FE6h
    db      000h,000h,000h,000h,000h,000h,000h,000h,000h,000h ; 1FF6h
