; Настройка клавиатурных часов в MAIN IO. Аппаратный профиль MX.

;----------------------------------------------------------------------------
; MXOS
;
; Запустить каскад клавиатурных часов и подготовить несущую щелчка.
;
; вход:
;   DOS зарезервировал владельца. Процедура исполняется в MAIN.
;
; выход:
;   CF=0, ZF=0 при двух изменениях счёта. CF=1, ZF=1 при тайм-ауте.
;   BC и DE сохранены. A, HL и остальные флаги меняются.
;----------------------------------------------------------------------------

ioKeyClockInit:
    ld hl,IO_TIMER_MODE
    ld (hl),036h
    ld (hl),074h
    dec hl
    dec hl
    ld (hl),0
    ld (hl),1
    inc hl
    inc hl
    ld (hl),0B0h
    dec hl
    ld (hl),1
    ld (hl),0
    push bc
    push de
    ld bc,0200h
    call io_pitRead
    ld e,l
.wait:
    call io_pitRead
    ld a,l
    sub e
    jp z,.next
    ld e,l
    dec b
    jp z,.ready
.next:
    dec c
    jp nz,.wait
    scf
    jp .done
.ready:
    ld hl,IO_TIMER_CH0
    ld (hl),low KEY_CLICK_DIVISOR
    ld (hl),high KEY_CLICK_DIVISOR
    or a
.done:
    pop de
    pop bc
    ret
