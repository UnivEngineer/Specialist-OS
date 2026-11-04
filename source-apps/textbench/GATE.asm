; Общий участок FFC0h–FFDFh, видимый в основной памяти и ДОЗУ.
; Вход: C = символ, активна страница 0, прерывания запрещены.
; Выход: страница 0, все регистры, флаги и стек сохранены.
; Не реентерабелен; временно заменяет штатный драйвер в общем окне.
; BIOS сохраняет все регистры. У него отдельный стек основного ОЗУ.
; 29 байт кода, 166 дополнительных тактов 8080 на вызов относительно
; прямого CALL BIOS (включая внутренний CALL, без внешнего CALL).
            include "../../source/include/mxos.inc"
            org 0FFC0h
gate:       push af
            push hl
            ld hl, 0
            add hl, sp
            ld (savedSP), hl
            ld (IO_PAGE_RAM), a
            ld sp, STACK_ADDR
service:    call bios_printChar
            xor a
            ld (IO_PAGE_RAMD), a
            ld hl, (savedSP)
            ld sp, hl
            pop hl
            pop af
            ret
            assert $ = 0FFDDh
            db 0
savedSP:    dw 0
            assert $ = 0FFE0h
