;+---------------------------------------------------------------------------
; MXOS
; ����������� ��� ����� ��� ������� ��
;
; �� ����� 
;  hl - �������� ���
;  de - ����� ��� ���������� (P_INPUT_WIDTH + 3 ����)
;
; 2013-12-12 ����������������� vinxru
;----------------------------------------------------------------------------

fileNamePrepare:
            ; ��������� ��������
            push    bc
            push    de

            ; ���� ������ ������ �������� ������ ':', �� ������ ����
            inc     hl          
            ld      a, (hl)
            cp      ':'
            dec     hl
            jp nz,  fileNamePr_1

            ; ������ ����
            ld      a, (hl)       
            sub     'A'
            push    hl
            call    fileSelectDrive
            pop     hl

            ; ���� �� ������ � ��� �����
            inc     hl          
            inc     hl          
            
fileNamePr_1:
            ; b = ����� �����, c = 3 - ����� ����������
            ld      bc, (FILE_NAME_LENGTH << 8) + 3

            ; �������� ����� �� �����, ������� ��� ����� ������, �� ����� FILE_NAME_LENGTH ��������.
fileNamePr_2:
            ld      a, (hl)
            and     a
            jp p,   fileNamePr_3
            sub     40h         ; ��������������� KOI?
fileNamePr_3:
            cp      ' '+1
            jp c,   fileNamePr_5
            inc     hl
            cp      '.'
            jp z,   fileNamePr_5            
            ld      (de), a
            inc     de
            dec     b
            jp nz,  fileNamePr_2

            ; ���������� ����� �� �����, ������� ��� ����� ������
fileNamePr_4:
            ld      a, (hl)
            cp      ' '+1
            jp c,   fileNamePr_6
            inc     hl
            cp      '.'
            jp nz,  fileNamePr_4
            jp      fileNamePr_6

; ---------------------------------------------------------------------------

fileNamePr_5:
            ; ��� ������ FILE_NAME_LENGTH ��������
            ; ��������� ����������� ������� ����� ���������
            call    memset_de_20_b

            ; �������� ����� �� ������� ��� ����� ������, �� ����� 3 ��������. 
fileNamePr_6:
            ld      a, (hl)                         
            and     a
            jp p,   fileNamePr_7
            sub     40h         ; ��������������� KOI?
fileNamePr_7:
            cp      ' '+1
            jp c,   fileNamePr_11
            ld      (de),a
            inc     hl
            inc     de
            dec     c
            jp nz,  fileNamePr_6

fileNamePr_8:
            ; ���������� ����� �� ������� ��� ����� ������
            ld      a, (hl)
            cp      ' '
            jp c,   fileNamePr_10
            inc     hl
            jp nz,  fileNamePr_8
            
;fileNamePr_9:
;            ; ���������� ����� �� ������� ��� ����� ������
;            ; ������ �� ����������� ���
;            ld      a, (hl)
;            cp      ' '
;            jp c,   fileNamePr_10
;            inc     hl
;            jp z,   fileNamePr_9
;            dec     hl

fileNamePr_10:    ; ��������������� �������� � �������
            pop     de
            pop     bc
            ret

; ---------------------------------------------------------------------------

fileNamePr_11:
            ; ��������� ����������� ������� ���������� ���������
            ld      b, c
            call    memset_de_20_b

            ; ��������������� �������� � �������
            pop     de
            pop     bc
            ret

