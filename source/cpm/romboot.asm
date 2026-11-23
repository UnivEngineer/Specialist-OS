; Автозапуск неизменённого CPM.RKS из MX-страницы ROM.
;
; вход:
;   Управление передано по 0000h в MX-странице ПЗУ. По 0100h
;   расположен образ размером CPM_BODY_SIZE. Стек не требуется.
;
; выход:
;   Образ перенесён в C000h, переходник в 8000h. Управление передано
;   переходнику. Рабочие регистры и флаги меняются.
    ORG 0
    di
    ld hl,0100h
    ld de,0C000h
    ld bc,CPM_BODY_SIZE
copyBody:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,copyBody
    ld hl,gate
    ld de,08000h
    ld b,gateEnd-gate
copyGate:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,copyGate
    jp 08000h

;----------------------------------------------------------------------------
; Загрузчик CP/M
;
; Завершить перенос образа CP/M после отключения ROM.
;
; вход:
;   Код переходника уже скопирован на 8000h, тело размером CPM_BODY_SIZE лежит по
;   C000h. Стек не требуется.
;
; выход:
;   Запись в 0FFFCh переключает память. Тело скопировано в 0100h, управление передано
;   на 0100h без адреса возврата.
;----------------------------------------------------------------------------

gate:
    ld (0FFFCh),a
    ld hl,0C000h
    ld de,0100h
    ld bc,CPM_BODY_SIZE
gateCopy:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,08000h+gateCopy-gate
    jp 0100h
gateEnd:
    ASSERT 0C000h+CPM_BODY_SIZE <= 0FFC0h
    ASSERT $ <= 0100h
