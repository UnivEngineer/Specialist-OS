;+---------------------------------------------------------------------------
; MXOS
; ��������� ������ ����� (�����������)
;
; 2013-12-12 ����������������� vinxru
;----------------------------------------------------------------------------

scrollUp2:  ; �������� 246 ���� �� sp � H0 
            ld    c, 246 / 6
            ld    l, 0

scrollUp2_0:      ; ����������� 6 ���� �� sp � hl
            pop   de
            ld  (hl), e
            inc   hl
            ld  (hl), d
            inc   hl
            pop   de
            ld  (hl), e
            inc   hl
            ld  (hl), d
            inc   hl
            pop   de
            ld  (hl), e
            inc   hl
            ld  (hl), d
            inc   hl

            ; ����
            dec   c
            jp nz,  scrollUp2_0

            ; ��������� h
            ld    a, h
            
            ; ��������� 10 ���� 
            ld    hl, (bios_vars.inverse)
            push  hl
            push  hl
            push  hl
            push  hl
            push  hl

            ; ��������� ������� (sp += 20)
            ld    hl, 14h
            add   hl,sp
            ld    sp, hl

            ; ��������������� h
            ld    h, a

            ; ��������� �������
            inc   h

            ; ����
            dec   b
            jp nz,  scrollUp2

            ; �������������� ��������� � �����
            ld    hl, (bios_vars.oldSP)
            ld    sp, hl
            pop   af
            pop   bc
            ret

