;+---------------------------------------------------------------------------
; MXOS
; ��������� ������� �� ������
;
; �� ������
;  bc, de, hl - �����������
;
; 2013-12-12 ����������������� vinxru
;----------------------------------------------------------------------------

drawCursor: ; ���� ���������� 7-�� ��� bios_vars.cursorCfg & 0x80, �������
            ld    a, (bios_vars.cursorCfg)
            and   a
            ret m 

drawCursor2:      ; ������ ���������/�����
            inc   c
            
drawCursor3:      ; ��������� ��������
            push  hl
            push  bc
            push  de

            ; ������������ ����� �� ������
            call  calcCursorAddr
            ld    d, a

            ; ��������� ������� �� +2 �� -5 (��� ����� 654 �� 0 �� 7)
            ; b = bios_vars.cursorCfg
            ; e = ~((bios_vars.cursorCfg / 16) % 8 - 3) + l
            ld    a, (bios_vars.cursorCfg)
            ld    b, a
            rrca
            rrca
            rrca
            rrca
            and   7
            sub   3
            cpl
            add   a, l
            ld    e, a

            ; ����������� ����� �������
            ;  hl = 011111100b << (((~h) % 4 +1)*2)
            ld    a, h
            cpl
            and   3
            ld    hl, 011111100b ; FCh
            inc   a
drawCursor_0:     add   hl, hl
            add   hl, hl
            jp    drawCursor_1

