;----------------------------------------------------------------------------
; MXOS NC.COM
; Вывод инверсной строки
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

printStringInv:   ld    a, 0FFh
            ld    (bios_vars.inverse),a
            ld    (bios_vars.inverse+1),a
            call  bios_printStringReal
            xor   a
            ld    (bios_vars.inverse),a
            ld    (bios_vars.inverse+1),a
            ret

