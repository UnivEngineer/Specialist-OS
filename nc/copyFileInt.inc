;----------------------------------------------------------------------------
; MXOS NC.COM
; ����� ������� ��� ����������� � �������� �����
;
; 2013-12-18 ����������������� vinxru
;----------------------------------------------------------------------------

copyFileInt:	; �������� ���������� ����� � ������
		call	loadSelFileAt0
		push	d	; ��� �������� �����

		; �������������� ���	
		xchg
		lxi	h, v_input
		call	fileNamePrepare
		xchg

loc_D4E4:	; ��������� ���
		push	h

		; ������������� ����� ������������ �����
		lxi	d, 10
		dad	d
		lxi	d, 0
		mov	m, e
		inx	h
		mov	m, d

		; ��������� ����
		pop	h
		call	fileCreate

		; �������� ����� �������� �����
		pop	d
		mvi	c, 1
		call	fileGetSetAddr

		; �������, ���� ��� ������
		rnc

		; ������ ����
		call	drawWindow2

		; ����� "CAN'T CREATE FILE"
        ; ����� ������ ��� x = 60h
        ; �������� ������ ������ w = 18*3/2 = 27
        ; ���������� ������ x = 60h-27 = 45h
		lxi	h, 4580h
		shld	v_cursorY
		lxi	h, aCantCreateFile
		call	printString

		; ����� �����
		lxi	h, 5090h
		shld	v_cursorY
		lxi	h, v_input
		call	printStringInv

anyKey:		; ���� ������� ����� �������
		call	keyScan
		inr	a
		jz	anyKey

		; �����
		pop	h
		jmp	mainReload
