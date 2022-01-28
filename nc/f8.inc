;----------------------------------------------------------------------------
; MXOS NC.COM
; ������� F8 - �������� �����
;
; 2013-12-18 ����������������� vinxru
;----------------------------------------------------------------------------

F8:		; ������ ����
		call	drawWindowIfSel

		; ������� ������� "DELETE FROM A:"
        ; ����� ������ ��� x = 60h
        ; �������� ������ ������ w = 14*3/2 = 21
        ; ���������� ������ x = 60h-21 = 4Bh
		lxi	h, 4B5Eh
		shld	v_cursorY
		lxi	h, aDeleteFrom
		call	printString
		call	printSelDrive

		call	printInvSelFile

loc_D54C:	; ���� ������� Enter ��� ESC
		call	keyScan
		cpi	1Bh
		jz	mainReload
		cpi	0Dh
		jnz	loc_D54C

		; ���� ���� ������ ��� ������
		call	getSelectedFile		
		lxi	d, 9
		dad	d
		mov	a, m
		ani	1
		jz	loc_D595

		call	drawWindow2

		; ������� ����� "FILE IS PROTECTED"
		lxi	h, 4680h
		shld	v_cursorY
		lxi	h, aFileIsProtected
		call	printString

		; ������� v_input
		lxi	h, 5090h
		shld	v_cursorY		
		lxi	h, v_input
		call	printStringInv

loc_D581:				; CODE XREF: drawWindowIfSel+379j
		call	keyScan
		inr	a
		jnz	loc_D581

loc_D588:				; CODE XREF: drawWindowIfSel+386j
		call	keyScan
		cpi	1Bh
		jz	mainReload
		cpi	0Dh
		jnz	loc_D588

loc_D595:	; ������� ������� ����
		call	getSelectedFile
		call	fileDelete

		; ��������� ������� ������
		lxi	h, panelA_filesCnt
		call	readBytePanel
		dcr	a
		call	writeBytePanel

		jmp	mainReload
