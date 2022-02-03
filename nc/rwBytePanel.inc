;----------------------------------------------------------------------------
; MXOS NC.COM
; Реализация индексной адресации
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

; [hl + activePanel] = a

writeBytePanel:   push  bc
            push  hl
            push  af
            ld    c, a
            ld    a, (activePanel)
            and   a
            jp z, loc_D97E
            inc   hl
loc_D97E:   ld  (hl), c
            pop   af
            pop   hl
            pop   bc
            ret

; a = [hl + activePanel]

readBytePanel:    push  hl
            ld    a, (activePanel)
            and   a
            jp z, loc_D98C
            inc   hl
loc_D98C:   ld  a, (hl)
            pop   hl
            ret

