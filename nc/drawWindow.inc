;----------------------------------------------------------------------------
; MXOS NC.COM
; ��������� ������
;
; 2013-12-18 ����������������� vinxru
;----------------------------------------------------------------------------

drawWindowIfSel:; �������� ���-�� ������
        ld      hl, state.panelA_filesCnt
        call    readBytePanel

        ; ���� ������ ���, �������
        and     a
        pop     hl ; ����� ��������
        jp z,   mainDriveChanged
        push    hl
            

; ---------------------------------------------------------------------------

drawWindow1:
        COLOR   COLOR_DIALOG

        ; ������ ����
        ld      hl, g_window1
        call    draw
      
        ; ������ �������������
        ld      hl, 9F66h   ; x=120 y=102
        ld      b, 18       ; w=144 h=9
        jp      inverseRect

; ---------------------------------------------------------------------------

drawWindow2:
        COLOR   COLOR_DIALOG

        ; ������ ����
        ld      hl, g_window2
        call    draw

        ; ������ �������������
        ld      hl, 9E88h   ; x=112 y=136
        ld      b, 20       ; w=160 h=9
        jp      inverseRect
