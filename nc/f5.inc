;----------------------------------------------------------------------------
; MXOS NC.COM
; ������� F5 - �����������
;
; 2013-12-18 ����������������� vinxru
;----------------------------------------------------------------------------

F5:		; ������ ����
		call    drawWindowIfSel

		; ������ ������� "COPYING FROM A: TO"
        ; ����� ������ ��� x = 60h
        ; �������� ������ ������ w = 15*3/2 = 22
        ; ���������� ������ x = 60h-22 = 4Ah
		lxi     h, 4A5Eh
		shld    v_cursorY
		lxi     h, aCopyFromTo 
		call    printString
		call    printSelDrive

		call    inputForCopyMove
		cpi     1Bh
		jz      mainReload

		call    copyFileInt
		jmp     mainReload
