;----------------------------------------------------------------------------
; MXOS NC.COM
; ������ �� ����� �������
;
; ����:
;   hl - ������
;   de - ����
;
; ����������� ��������:
;   hl, bc
;
; 2013-12-18 ����������������� vinxru
;----------------------------------------------------------------------------

compactName:
            ; ��������� ��������
            push    hl
            push    bc

            ; �������� �� �������, �� �� ����� FILE_NAME_LENGTH ����
            ld      bc, FILE_NAME_LENGTH
compactName_1:      ld  a, (hl)
            cp      ' '
            jp z,   compactName_2           
            ld      (de),a
            inc     hl
            inc     de
            dec     c
            jp nz,  compactName_1

compactName_2:
            ; ����������� ������� �������� ������
            add     hl, bc

            ; �����
            ld      a, '.'
            ld      (de),a
            inc     de

            ; �������� �� �������, �� �� ����� 3 ����
            ld      c, 3
compactName_3:
            ld      a, (hl)
            cp      ' '
            jp z,   compactName_4
            ld      (de),a
            inc     hl
            inc     de
            dec     c
            jp nz,  compactName_3

compactName_4:
            ex      de, hl
            push    af

            ; ���� ��������� ������ '.', �� ������� ���
            dec     hl
            ld      a, (hl)
            cp      '.'
            jp z,   compactName_5
            inc     hl

compactName_5:
            ; ��������� ������ � ����� ������
            ld      (hl), ' '
            inc     hl
            ld      (hl), 0Dh
            inc     hl
            ld      (hl), 0

            ; ��������� � de
            ex    de, hl

            ; �������������� �������� � �������
            pop   af
            pop   bc
            pop   hl
            ret
