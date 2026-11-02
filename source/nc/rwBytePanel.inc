;----------------------------------------------------------------------------
; MXOS NC.COM
; Запись значения в переменную панели
; [hl + state.activePanel] = a
;
; вход:
;   hl = адрес переменной панели A (state.panelA_info, state.panelA_drive и т.п.)
;   a  = значение переменной
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

writeBytePanel:
            push    hl
            push    af
            ld      a, (state.activePanel)
            or      a
            jp z,   wbp_Store
            inc     hl
wbp_Store:
            pop     af
            ld      (hl), a
            pop     hl
            ret

;----------------------------------------------------------------------------
; MXOS NC.COM
; Чтение значения из переменной панели
; a = [hl + state.activePanel]
;
; вход:
;   hl = адрес переменной панели A (state.panelA_info, state.panelA_drive и т.п.)
;
; выход:
;   a = значение переменной
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

readBytePanel:
            push  hl
            ld    a, (state.activePanel)
            and   a
            jp z, loc_D98C
            inc   hl
loc_D98C:   ld    a, (hl)
            pop   hl
            ret

