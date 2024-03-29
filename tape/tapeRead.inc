;+---------------------------------------------------------------------------
; MXOS
; ������ ����� � �����
;
; �� �����
;  a=255 ������� �����-���� �� ����� �� E6h, �������� ������� ����� �����
;  a=8   �������� ����� (8 ���)
;
; �� ������
;  c - ����
;  de, hl - �����������
;
; 2013-12-12 ����������������� vinxru
;----------------------------------------------------------------------------

tapeRead:   ; ��������� ��������
            push  bc
            push  de

            ; ��� ����� �������� ����
            ld    c, 0

            ; d=8 ���� �������� (������� ���) ��� 0FFh ���� ��������
            ld    d, a

tapeRead_0: ; �������� ���������
            ld    a, (IO_KEYB_B)
            and   1
            ld    e, a

            ; �������� c
            ld    a, c
            and   7Fh
            rlca
            ld    c, a

tapeRead_1: ; �������� ���������
            ld    a, (IO_KEYB_B)

            ; ���� ������ �������, �������
            cp    80h
            jp c, tapeReadError

            ; ���� ��������� �������
            and   1
            cp    e
            jp z, tapeRead_1

            ; ��������� ���
            or    c
            ld    c, a

            ; ��������
            call  readDelay

            ld    a, (IO_KEYB_B)
            and   1
            ld    e, a

            ; ���� ���������� �������� ������, ��������� �� tapeRead_4
            ld    a, d
            or    a
            jp p, tapeRead_4

            ; ���� ���������� ��������
            ; ���� �� ������ 0E6h, ��������� tapeRead_2
            ld    a, c        
            cp    0E6h
            jp nz,  tapeRead_2

            ; �������� �������� ��� ��������
            xor   a
            ld    (bios_vars.tapeInverse),a
            jp    tapeRead_3

tapeRead_2: ; ���� �� ������ 19h, ��������� �� tapeRead_0
            cp    19h
            jp nz,  tapeRead_0

            ; �������� �������� � ���������
            ld    a, 0FFh
            ld    (bios_vars.tapeInverse),a

            ; ��������� 8 ���
tapeRead_3: ld    d, 9

            ; ��������� 8 ����
tapeRead_4: dec   d
            jp nz,  tapeRead_0

            ; �������� �����
            ld    a, (bios_vars.tapeInverse)
            xor   c

            ; �������������� ��������
            pop   de
            pop   bc
            ret

