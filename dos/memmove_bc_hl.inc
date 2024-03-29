;+---------------------------------------------------------------------------
; MXOS
; ����������� ��������������� ������ ������ (� �������)
;
; �� �����
;  hl - ������
;  de - ������, �������� ����� �� �������
;  bc - ����, � ����������� �������
;
; 2013-12-12 ����������������� vinxru
;----------------------------------------------------------------------------

memmove_bc_hl:
            ; ��������� ��������
            push    hl
            push    bc
            push    de

            ; ���� b<h ��� c<l, ��
            ld      a, b
            cp      h
            jp c,   memcpy_bc_hl2

            ; ��� �� ������� jnz

            ld      a, c
            cp      l
            jp c,   memcpy_bc_hl2

            ; bc = bc + de - hl
            push    hl          
            call    sbb_de_hl_to_hl
            add     hl, bc
            ld      b, h
            ld      c, l
            pop     hl

memcpyb_bc_de:
            ; �������� �� de � bc � ����������� �������, ���� hl �� ����� de
            ld      a, (de)
            ld      (bc), a
            call    cmp_hl_de_2
            dec     de
            dec     bc
            jp nz,  memcpyb_bc_de

            ; �������������� ��������� � �����
            jp      popDBH_ret2

memcpy_bc_hl2:
            ; �������� �� hl � bc � ����������� �������, ���� hl �� ����� de
            ld      a, (hl)
            ld      (bc),a
            call    cmp_hl_de_2
            inc     hl
            inc     bc
            jp nz,  memcpy_bc_hl2

popDBH_ret2:
            ; �������������� ��������� � �����
            pop     de
            pop     bc
            pop     hl
            ret

