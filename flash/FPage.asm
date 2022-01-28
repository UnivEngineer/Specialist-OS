;+---------------------------------------------------------------------------
; MXOS
; FLPAGE.COM - ������������� ������� ����-����� ��55
;
; 2022-01-25 ����������� SpaceEngineer
;
;----------------------------------------------------------------------------

; ������� �������
printChar       = 0C809h ; ����� ������� �� �����
input           = 0C80Fh ; ���� ������ � ����������
printHexByte	= 0C815h ; ������� 16-������ ����� (����)
printString     = 0C818h ; ������� ������ �� �����
strToHex        = 0C839h ; ������������� ������ � HEX ������� � �����

; ���������� �������
v_flashPage     = 8FF9h  ; ������� �������� ����-����� (����. 1Fh = 31)
v_input         = 0DE8Eh ; ����� ��� ����� ������

.org 08000H

        ; ����� ������� ��������
	    lxi  h, txtCurrentPage
	    call printString
        lda  v_flashPage
        call printHexByte

Repeat:
        ; ����� �������
	    lxi  h, txtEnterPage
	    call printString

        ; ���� ������
	    lxi  h, v_input
	    lxi  d, v_input+23
        call input

        ; ������ ������
        xchg
        call strToHex
        jz   Repeat

        ; ��������� ��������
        lxi  d, 20h ; ����. 1Fh + 1
        call cmp_hl_de
        jnc  Repeat

        ; ���������� ���������
        mov a, l
        sta v_flashPage
        ret

cmp_hl_de:
        mov	a, h
        cmp	d
        rnz
        mov	a, l
        cmp	e
        ret

txtCurrentPage:
        .db 0Ah,"CURRENT PAGE NUMBER: ",0

txtEnterPage:
        .db 0Ah,"ENTER NEW PAGE NUMBER (0-1F): ",0

.END
