; A/CF = клавиша и признак CTRL из bios_getch. AF/BC/DE/HL разрушены.
; CF=1: не снимать выделение; CF=0: обычный обработчик должен его снять.
; scrollCursorKept=1 только для однострочной прокрутки на том же месте экрана.
keepCursorForKey:
            ld      c, a
            ld      a, 0            ; флаг CTRL пока сохраняется
            ld      (vars.scrollCursorKept), a
            ret c
            ld      a, c
            cp      0Bh
            jp c,   kcf_Erase       ; F1-F8, LEFT, TAB, END
            jp z,   kcf_Keep        ; неиспользуемый код
            cp      0Eh
            jp c,   kcf_Erase       ; HOME, ENTER
            cp      18h
            jp c,   kcf_Keep        ; неиспользуемые коды
            cp      1Bh
            jp nc,  kcf_Keep        ; ESC, текст, BACKSPACE и F9
    IF NC_BITMAP_SCROLL
            cp      19h
            jp z,   kcf_Up
            cp      1Ah
            jp z,   kcf_Down
    ENDIF
kcf_Erase:
            or      a
            ret
kcf_Keep:
            scf
            ret
    IF NC_BITMAP_SCROLL
kcf_Up:
            ld      hl, state.panelA_curFile
            call    readBytePanel
            or      a
            jp z,   kcf_Erase
            ld      b, a
            ld      hl, state.panelA_firstFile
            call    readBytePanel
            cp      b
            jp z,   kcf_KeepScroll
            jp      kcf_Erase
kcf_Down:
            ld      hl, state.panelA_filesCnt
            call    readBytePanel
            ld      c, a
            ld      hl, state.panelA_curFile
            call    readBytePanel
            inc     a
            cp      c
            jp nc,  kcf_Erase
            dec     a
            ld      b, a
            ld      hl, state.panelA_firstFile
            call    readBytePanel
            add     FILE_LIST_SIZE-1
            cp      b
            jp nz,  kcf_Erase
kcf_KeepScroll:
            ld      a, 1
            ld      (vars.scrollCursorKept), a
            scf
            ret
    ENDIF
