;+---------------------------------------------------------------------------
; MXOS
; ������������ (�����������)
;
; 2013-12-12 ����������������� vinxru
;----------------------------------------------------------------------------

reboot2:
		; ������� �����
		call	clearScreen

		; ���������, ������ �� ����� ������ �������
		call	j_keyScan

		push	psw

		; ������� ������ �� �� �����
		lxi h, txtBiosVer
		call printString1

		; ���������� ������ ���� � �������� ������� �� �����
		lxi	 h, ARAM_PAGE_END
		lda	 v_maxRamPage
		call checkARAM

		; ������� ����� ������ �� �����
		lxi  h, txtRAM
		call printString1

		lda	 v_maxRamPage
        call printHexByte

		lxi  h, txtKB
		call printString1

        ;call getch

		pop	psw

		; ���� �� ������ �� ���� �������, ��������� ���� � RAM-�����
		inr	a
		jz	j_reboot3

		; ��������� ��������� � ����� (��� �������� ��) � ���������
		call	tapeLoadInt
		lhld	v_tapeAddr
		pchl

; ---------------------------------------------------------------------------

txtBiosVer:
		.db 0Ch	; '\f'
		.text "MXOS BIOS 4.50"
		.db 0Ah	; '\n'
		.db 0

txtRAM:
		.db 0Ah	; '\n'
		.text "RAM "
		.db 0

txtKB:
		.text " * 64 KB"
		.db 0Ah	; '\n'
		.db 0