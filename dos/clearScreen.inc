;+---------------------------------------------------------------------------
; MXOS
; �������� �����
;
; �� ������
;  bc, de, hl - �����������
;
; 2013-12-12 ����������������� vinxru
;----------------------------------------------------------------------------

clearScreen:
            ; bc, hl - ���������. de - �� ������������
            push  hl
            push  bc

            ; ���������� sp
            ld    hl, 0
            add   hl,sp
            ld    (bios_vars.oldSP), hl

            ; ������������� sp � ����� �����������
            ld    sp, 0C000h

            ; ���� (�����) ��� ���������� ������
            ld    hl, (bios_vars.inverse)

            ; �������� � ���� 3000h ����
            ld    bc, 3000h / 16
clearScreen_0:
            push      hl
            push      hl
            push      hl
            push      hl
            push      hl
            push      hl
            push      hl
            push      hl
            dec bc
            ld  a, b
            or  c
            jp nz,  clearScreen_0

            ; ��������������� sp
            ld    hl, (bios_vars.oldSP)
            ld    sp, hl

            ; bc, hl ���� ���������. de - �� ������������
            pop   bc
            pop   hl
            ret

