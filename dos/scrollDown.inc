;+---------------------------------------------------------------------------
; MXOS
; ��������� ������ ����
;
; �� ������
;   af, bc - �����������
;
; 2013-12-12 ����������������� vinxru
;----------------------------------------------------------------------------

            ; ��������� ��������
            push    bc
            push    af
            ld      hl, 0
            add     hl,sp
            ld      (bios_vars.oldSP), hl

            ; ���������� ����������
            ld      sp, 0C000h    ; ����
            ld      h, 0BFh       ; ������
            ld      b, 48         ; ��������
loc_C741:   ld      c, 246 / 6    ; 246 �����
            ld      l, 245        ; �������� � 245 ������

loc_C745:   ; �������� 6 ���� �� hl � sp
            ld      d, (hl)
            dec     hl
            ld      e, (hl)
            dec     hl
            push    de          
            ld      d, (hl)
            dec     hl
            ld      e, (hl)
            dec     hl
            push    de
            ld      d, (hl)
            dec     hl
            ld      e, (hl)
            dec     hl
            push    de

            ; ����
            dec     c
            jp nz,  loc_C745

            ; ��������� h
            ld      a, h
            
            ; ������� ������� 10 �����
            ld      hl, (bios_vars.inverse)
            push    hl
            push    hl
            push    hl
            push    hl
            push    hl

            ; ��������������� h
            ld      h, a

            ; ��������� �������
            dec     b
            jp nz,  loc_C741

            ; �������������� �������� � �������
            ld      hl, (bios_vars.oldSP)
            ld      sp, hl
            pop     af
            pop     bc
            ret

