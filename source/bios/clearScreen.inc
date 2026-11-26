;+---------------------------------------------------------------------------
; MXOS
; Очистить экран
;
; выход:
;  bc, de, hl - сохраняются
;
; 2013-12-12 Дизассемблировано vinxru
;----------------------------------------------------------------------------

clearScreen:
            ; bc, hl - сохраняем. de - не используется
            push  hl
            push  bc

            ; Сохранение sp
            ld    hl, 0
            add   hl,sp
            ld    (bios_vars.oldSP), hl

            ; Устанавливаем sp в конец видеопамяти
            ld    sp, MX_SCREEN_END

            ; Байт (слово) для заполнения памяти
            ld    hl, (bios_vars.inverse)

            ; Помещаем в стек 3000h байт
            ld    bc, MX_SCREEN_BYTES / 16
clearScreen_0:
            push      hl
            push      hl
            push      hl
            push      hl
            push      hl
            push      hl
            push      hl
            push      hl
            dec bc
            ld  a, b
            or  c
            jp nz,  clearScreen_0

            ; Восстанавливаем sp
            ld    hl, (bios_vars.oldSP)
            ld    sp, hl

            ; bc, hl были сохранены. de - не используется
            pop   bc
            pop   hl
            ret

