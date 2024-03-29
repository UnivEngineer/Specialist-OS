;+---------------------------------------------------------------------------
; MXOS
; ��������� ��������� ����� ��� ������ ������� (�����������)
; ������ ������ ������� � ��������������� � ������ ������ �� �����
;
; 2013-12-12 ����������������� vinxru
;----------------------------------------------------------------------------

; ---------------------------------------------------------------------------
; ��������� ESC+(. ��������� KOI-7

printChar_e28:
            ld    a, 0FFh
            ld    (bios_vars.koi7), a
            ret

; ---------------------------------------------------------------------------
; ��������� ESC+). ��������� KOI-8

printChar_e29:
            xor   a
            ld    (bios_vars.koi7), a
            ret

; ---------------------------------------------------------------------------
; ��������� ���� 9. ���������

printChar_c9:
            ld    a, (bios_vars.cursorX)  
            add   a, 12 ; 24
            ld    (bios_vars.cursorX),a
            ret

; ---------------------------------------------------------------------------
; ����� ������� � �� �������������� �� ������ hl*8 �� ����� � ��������� 
; �������. ������ ��� ���� ������������ 

printChar_alt:
            ; de = (hl+c) * 8
            ld    b, 0
            add   hl, bc
            add   hl, hl
            add   hl, hl
            add   hl, hl
            ex    de, hl

            ; ����������� � printChar_de

; ---------------------------------------------------------------------------
; ����� ������� de �� ����� � ��������� �������. ������ ��� ���� ������������ 
; ������.

printChar_de:
            ; ������������ ����� ������ (hl=v_cursorPos, a*256+l=�����)
            call  calcCursorAddr

            ; ���������� ������ ������
            inc   h
            inc   h
            inc   h           
            ld    (bios_vars.cursorY), hl

            ; ������������ ����� ������ (bc = al - 7)
            ld    b, a
            ld    a, l
            sub   7
            ld    c, a

            ; ����������� � drawChar

