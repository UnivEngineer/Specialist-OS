;----------------------------------------------------------------------------
; MXOS NC.COM
; ��������� ������
;
; 2013-12-18 ����������������� vinxru
;----------------------------------------------------------------------------

drawWindowIfSel:
        ; �������� ���-�� ������
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
        ld      hl, 9F65h           ; x=120 y=101
        ld      bc, (18 << 8) + 11  ; w=144 h=11
        jp      inverseRect

; ---------------------------------------------------------------------------

drawWindow2:
        COLOR   COLOR_DIALOG

        ; ������ ����
        ld      hl, g_window2
        call    draw

        ; ������ �������������
        ld      hl, 9E87h           ; x=112 y=135
        ld      bc, (20 << 8) + 11  ; w=160 h=11
        jp      inverseRect
