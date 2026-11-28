;----------------------------------------------------------------------------
; MXOS NC.COM
; Вывод инверсной строки
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

printStringInv:   ld    a, 0FFh
            ld    (mon2_vars.inverse),a
            ld    (mon2_vars.inverse+1),a
            call  FONT_ENGINE_RAWZ
            xor   a
            ld    (mon2_vars.inverse),a
            ld    (mon2_vars.inverse+1),a
            ret

