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

            ; ������������ ���������� X (������, �����)
            ld    h, 90h + (P_FILE_LIST_X1 * 2 / 8)
            cp    FILE_LIST_SIZE/2
            jp c, drawCursor_1
            sub   FILE_LIST_SIZE/2
            ld    h, 90h + (P_FILE_LIST_X2 * 2 / 8)

drawCursor_1:
            ; ������������ ���������� Y (������, �����)
            ; a = a * 10 + P_FILE_LIST_Y - 8
            add   a, a
            ld    c, a
            add   a, a
            add   a, a
            add   a, c
            add   P_FILE_LIST_Y - 8
            ld    l, a

            ; ������ ������� � ������, ������ � ��������
            ld    bc, (10 << 8) + 10

            ; ���� ������ ������, �������� ������ �� ��� ������
drawCursor_2:
            ld    a, (state.activePanel)
            and   a
            jp z, drawCursor_3
            ld    a, 18h
drawCursor_3:
            add   a, h
            ld    h, a

            ; ������ ������ (hl = �����, b = ������, c = ������)
            call  inverseRect
            ret

; ---------------------------------------------------------------------------

    IF ENABLE_COLOR==0
drawCursor_0:
            ld    hl, 92D6h
            ld    b, 2
            jp    drawCursor_2
    ENDIF

