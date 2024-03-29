;+---------------------------------------------------------------------------
; MXOS
; ������ ��������� �� �����
;
; �� �����
;  hl - ��������� �����
;  de - �������� �����
;
; �� ������
;  bc - ����������� �����
;  de, hl - �����������
;
; 2013-12-12 ����������������� vinxru
;----------------------------------------------------------------------------

tapeSave:   ; ����������� CRC
            push    hl
            call    bios_calcCS
            pop     hl

            push    hl
            push    bc

            ; �����-��� (256 �����)
            ld      b, 0
loc_C9A4:   xor     a
            call    tapeWrite
            dec     b
            jp nz,  loc_C9A4

            ; ��������� ���� 0E6h
            ld      a, 0E6h
            call    tapeWrite

            ; ������ ������ ������� �����
            call    tapeWriteWord
            ex      de, hl

            ; ������ ������ ���������� �����
            call    tapeWriteWord
            ex      de, hl
            
loc_C9B9:   ; ������ ����� ������ �� hl �� de
            ld      a, (hl)
            call    tapeWrite
            call    cmp_hl_de_2
            inc     hl
            jp nz,  loc_C9B9

            ; ������ CRC
            pop     hl
            call    tapeWriteWord

            ; ���������� CRC � �������� bc
            ld      b, h
            ld      c, l

            ; ��������������� �������� � �������
            pop     hl
            ret

