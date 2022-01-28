;----------------------------------------------------------------------------
; MXOS NC.COM
; ������
;
; 2013-12-18 ����������������� vinxru
;----------------------------------------------------------------------------

Start:		; ��������� ��������
		lxi	h, 0
		shld	v_inverse

		; *** ���������� �������� FORMAT ***

		; ������������� ���� �������
		call	swapDriver

		; ���� � ���-����� �� ������ 0FF00h (��������� �������� ������)
        ; ���� ����������� �����, ������ ��� ���������
		lxi h, 0FF00h
		call	0FFC0h ; ������ 1 ���� �� ���-����� �� ������ hl
		cpi	5Ah
		jz	formatCalled

		; ��������������� �������
		call	swapDriver

		; ��������� ���������
		call	saveState

		; � ���������: ������ "A:FORMAT.COM B:"
		; SpaceEngineer: ������ ����� ��������� "A:FORMAT.BAT"
		lxi	h, aFormatB
		jmp	fileExec

; ---------------------------------------------------------------------------

formatCalled:	; �������� ��������� �� ���� � ���
		inx	h
		lxi	d, activePanel
		mvi	c, 9    ; 9 ����
loc_D026:
        call	0FFC0h
		stax	d
		inx	h
		inx	d
		dcr	c
		jnz	loc_D026

		; ��������������� �������
		call	swapDriver

        ; ������� ������
		mvi	c, 1Fh

		; �����	���������
		COLOR(COLOR_HELP_F)
		call	printChar
		lxi	h, 1FFh
		shld	v_cursorY
		lxi	h, aF1LeftF2RighF3		
		call	printString

		; ������������ ���������
		COLOR(COLOR_HELP_TEXT)
		lxi	h, 92F7h
#if SHOW_F9
        mvi	d, 9
#else
        mvi	d, 8
#endif
loc_D049:
#if SHOW_F9
		mvi	b, 3
#else
        mvi	b, 4
#endif
		call	inverseRect	; hl - �����, b	- ������, ������ 9
		inr	h
		inr	h
		dcr	d
		jnz	loc_D049

		; ������� ��� ������
		call	clearCmdLine
