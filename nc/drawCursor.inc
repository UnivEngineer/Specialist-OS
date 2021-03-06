;----------------------------------------------------------------------------
; MXOS NC.COM
; ��������� �������
;
; 2013-12-18 ����������������� vinxru
;----------------------------------------------------------------------------

drawCursor: ; ���� �������������� ����� �������

            ; ���� ������ ���, �������
            ld    hl, state.panelA_filesCnt
            call  readBytePanel
            and   a
    IF ENABLE_COLOR
            ret Z 
    ELSE
            jp z, drawCursor_0
    ENDIF
            ; C ������ ����� ���������� ������ ������ �� ������
            ld    hl, state.panelA_firstFile
            call  readBytePanel
            ld    c, a

            ; ����� ���������� �����
            ld    hl, state.panelA_curFile
            call  readBytePanel
            sub   c

            ; ������������ ���������� X (h = a < 18 ? 0x91 : 0x9D)
            ld    h, 91h
            cp    18
            jp c, drawCursor_1
            sub   18
            ld    h, 9Dh

drawCursor_1:
            ; ������������ ���������� Y (l = a * 10 + 18h)
            add   a, a
            ld    c, a
            add   a, a
            add   a, a
            add   a, c
            add   a,24
            ld    l, a

            ; ������
            ld    b, 10

            ; ���� ������ ������, �������� ������ �� ��� ������
drawCursor_2:     ld    a, (state.activePanel)
            and   a
            jp z, drawCursor_3
            ld    a, 18h
drawCursor_3:     add   a, h
            ld    h, a

            ; ������ ������ (hl - �����, b - ������, ������ 9)
            call  inverseRect
            ret

; ---------------------------------------------------------------------------

    IF ENABLE_COLOR==0
drawCursor_0:     ld    hl, 92D6h
            ld    b, 2
            jp    drawCursor_2
    ENDIF

