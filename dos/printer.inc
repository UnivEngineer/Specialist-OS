;+---------------------------------------------------------------------------
; MXOS
; ����� ����� �� �������
;
; �� ����� 
;   c - ����
;
; �� ������ 
;   af, bc, de, hl - c����������
;
; 2013-12-12 ����������������� vinxru
;----------------------------------------------------------------------------

printer:    ; ��������� ��������
            push  af

            ; ����� ����� ����������
            ld    a, 90h
            ld    (IO_PROG_MODE), a

            ; ������� ���� �� �������
            ld    a, c        
            cpl
            ld    (IO_PROG_B), a

printer_loop:     ; �����     ��� ������� ����� �������
            ld    a, (IO_KEYB_B)     
            and   2
            jp z, printer_ret

            ; ���� ���� ������� �� ����� �����
            ld    a, (IO_PROG_A)       
            and   40h
            jp nz,  printer_loop
            
            ; �����
            ld    a, 3        
            ld    (IO_PROG_C), a
            xor   a
            ld    (IO_PROG_C), a

printer_ret:      ; �������������� ��������
            pop   af
            ret

