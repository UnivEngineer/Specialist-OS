;----------------------------------------------------------------------------
; MXOS NC.COM
; ������� F9 - ���������� ����� �� ����������
;
; 2013-12-18 ����������������� vinxru
;----------------------------------------------------------------------------

f9:		; ������ ����
		call	drawWindowIfSel

		; ������� SAVING FROM A: TO TAPE
        ; ����� ������ ��� x = 60h
        ; �������� ������ ������ w = 20*3/2 = 30
        ; ���������� ������ x = 60h-30 = 42h
		lxi	h, 425Eh
		shld	v_cursorY
		lxi	h, aSaveFromToTape
		call	printString
		call	printSelDrive

		; ?
		call	printInvSelFile

		; ���� ������� Enter ��� ESC.
loc_D39C:	call	keyScan
		cpi	1Bh
		jz	mainReload
		cpi	0Dh
		jnz	loc_D39C

		; ���������� SP	� ��������� ���	������.
		call	prepareTapeLoad

		call	drawWindowIfSel

		; ����� SAVING TO TAPE
        ; ����� ������ ��� x = 60h
        ; �������� ������ ������ w = 14*3/2 = 21
        ; ���������� ������ x = 60h-21 = 4Bh
		lxi	h, 4B5Eh
		shld	v_cursorY
		lxi	h, aSavingToTape	
		call	printString

		; �������� �����
		call	loadSelFileAt0

		; ��������� ������ �������� � �����
		push	d
		push	h
		lxi	d, 0Ch
		dad	d
		mov	e, m
		inx	h
		mov	d, m
		pop	h
		push	d
		push	h
		lxi	h, 0
		dad	d
		xchg
		lxi	h, 0

		; ������ ����������� �����
		call	calcCS
		mov	h, b
		mov	l, c
		shld	v_tapeSaveCRC

		; �����-���
		call	tapeWritePilot

		; ������ �����
		mvi	a, 0D9h
		call	tapeWrite
		call	tapeWrite
		call	tapeWrite

		pop	h ; ������
		lxi	b, 8 ; �����
		call	tapeWriteBlock

		; �����-���
		call	tapeWritePilot

		pop	b ; �����
		pop	h ; ��������� �����
		call	tapeWriteWord

		dad	b ; �������� �����		
		call	tapeWriteWord

		; ������ �����
		lxi	h, 0
		call	tapeWriteBlock	; hl-�����, bc-�����-1

		; ������ ����������� �����
		lhld	v_tapeSaveCRC
		call	tapeWriteWord

		jmp	mainReload
