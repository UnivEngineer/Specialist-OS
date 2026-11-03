; Адаптер координат NC к общему оконному ABI DOS.
; HL = список объектов. AF/BC/DE/HL и флаги разрушаются.
draw:
            ld      a, (state.activePanel)
            or      a
            jp z,   drawPanelOffset
            ld      a, 24
drawPanelOffset:
            jp      bios_uiDraw

; HL = экранный адрес, сместить на активную панель; AF сохраняется.
activePanelPos:
            push    af
            ld      a, (state.activePanel)
            or      a
            jp z,   activePanelOffset
            ld      a, 24
activePanelOffset:
            add     a, h
            ld      h, a
            pop     af
            ret
