;+---------------------------------------------------------------------------
; MXOS
; ��������� ������� �� ������ (�����������)
;
; 2013-12-12 ����������������� vinxru
;----------------------------------------------------------------------------

drawCursor_1:     ; ����
            dec   a
            jp nz,  drawCursor_0

            ; b = vars.cursorCfg & 0xF
            ld    a, b
            and   0Fh
            ld    b, a

            ; ����� ������� (b - ������, de - ����� �� ������, hl - ������)

drawCursor_2:     ; ��������� ��� ���� b ���
            dec   b
            jp M, popa_ret_2

            ; *de--     ^= hl;
            ld    a, (de)
            xor   h
            ld    (de),a
            inc   d
            ld    a, (de)
            xor   l
            ld    (de),a
            dec   d
            dec   e
            jp    drawCursor_2

