;+---------------------------------------------------------------------------
; MXOS
; ������������� ������ � 16-������ �����
; ������������ �� RAMFOS
;
; �� �����
;  de - ������
;
; �� ������
;  bc       - �����������
;  de       - ��������� ������ �� ������� ��� ������ ������
;  jz       - ������
;  jnz, jc  - ����� ������
;  jnz, jnc - ������ ��� �������
;  hl       - �����
;
; 2013-11-01 ����������������� vinxru
;----------------------------------------------------------------------------

strToHex:
        lxi	h, 0		

strToHex_loop:	; ������ �������
		ldax	d
		inx	d

		; ���� ����� ������, ������� � ������� NZ, C
		cpi	0Dh
		jz	ora_a_stc_ret

		; ���� ������ ��� �������, ������� � ������� NZ, NC
		cpi	','
		jz	ora_a_ret		
		cpi	' '
		jz	ora_a_ret

		; ���� ������������ ������, ������� � ������� �,Z
		sui	'0'
		jm	xra_a_stc_ret		

		; ���� ��� �����
		cpi	10
		jm	strToHex_0

		; ���� ������������ ������, ������� � ������� �,Z
		cpi	11h
		jm	xra_a_stc_ret
		cpi	17h
		jp	xra_a_stc_ret

		; ��� ����� A..F
		sui	'A'-'0'-10

strToHex_0:	
		; hl *=	16
		dad	h
		dad	h
		dad	h
		dad	h

		; ���� ������������, ������� c ������� �,Z
		jc	xra_a_stc_ret
		
		; hl += bc
		push b
		mvi  b, 0
		mov  c, a
		dad  b
		pop  b

		jmp	strToHex_loop

xra_a_stc_ret:	; ����� ����� �������
        mvi	c, '?'
        call	printChar
        xra	a

ora_a_stc_ret:
        ora	a
        stc
        ret

ora_a_ret:
        ora	a
        ret