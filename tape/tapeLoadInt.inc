;+---------------------------------------------------------------------------
; MXOS
; �������� ��������� � �����. �� �� �����������.
;
; � ������ ������ ���������� ���������� �� ������ � bios_vars.tapeError
;
; �� ������ 
;  bios_vars.tapeAddr - ����� ����������� ���������
;
; 2013-12-12 ����������������� vinxru
;----------------------------------------------------------------------------

tapeLoadInt:
            ; ���� �����-��� � ������ ����� - ��� ����� �������� ���������
            ld      a, 0FFh
            call    tapeRead          
            ld      l, a
            ld      a, 8
            call    tapeRead
            ld      h, a
            ld      (bios_vars.tapeAddr), hl

            ; ������ ��������� ����� - ��� ����� ���������
            ld      a, 8
            call    tapeRead
            ld      e, a
            ld      a, 8
            call    tapeRead
            ld      d, a

            ; ������ ���� ���������
loc_C414:   ld      a, 8        
loc_C416:   call    tapeRead
            ld      (hl), a
            call    bios_cmp_hl_de
            inc     hl
            jp nz,  loc_C414

            ; ������� �� ��������
            ret

; ---------------------------------------------------------------------------
; �� ������������

tapeLoadAny:
            ld    a, 0FFh
            jp    loc_C416

