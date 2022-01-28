;----------------------------------------------------------------------------
; MXOS NC.COM
; ������� ENTER
;
; 2013-12-18 ����������������� vinxru
;----------------------------------------------------------------------------

Enter:  ; ���� ��� ������ �� ������
		lhld v_cmdLineEnd
		mvi  a, v_cmdLine & 0FFh
		cmp  l
		jnz  EnterCmdLine

        ; ��������� ������� ���� � �������� B
        mvi  e, 0
        call fileGetSetDrive
        mov  b, a

        ; ����������� ��� ����� A:NC.EXT
		lxi  h, aNcExt
		lxi  d, files2; files2 ������������ ��� �����
		call fileNamePrepare
        xchg

		; ��������� ���� A:NC.EXT �� ������ 0
		lxi  h, files2
		lxi  d, 0
		mvi  c, 1
		call fileGetSetAddr ; �������� ����� �������� ����� �� DE = 0
		call fileLoad
		jc   Enter_stdExec

        ; ����������� ���� �������
        ;mov  a, b
        ;mvi	 e, 1
        ;call fileGetSetDrive

		; �������� ��� ���������� �����
		call getSelectedFile
		mov  a, m
		inr  a
		jz   mainLoop ; ������ �� �������

		; hl - ���������� �����
		lxi	d, 6
		dad	d

		; hl - ���������� �����
		lxi	d, 0

		; ���������� ���������� �� �����
Enter_extLoop:
		mvi  c, 3
		push h
Enter_1:
		ldax d
		cmp  m
		jnz  Enter_nextExt
		inx  h
		inx  d
		dcr  c
		jnz  Enter_1
		pop  h
		inx  d

		; ������� ����������

		; ����������� ����� ���������� ����� � ��� ������
Enter_5:
		lxi	h, v_cmdLine
Enter_4:
		ldax d
		mov  m, a
		cpi  0Dh
		jz   Enter_3
		inr  a
		jz   Enter_3
		inx  h
		inx  d
		jmp  Enter_4

; ---------------------------------------------------------------------------

Enter_3:
		mvi  m, ' '
		inx  h
		mvi  m, 0
		shld v_cmdLineEnd

		; ����� ��� ������ �� �����
		lxi  h, v_cmdLine
		call printString

Enter_stdExec:	; �������� ��������� ����
		call getSelectedFile

		; ������ �� �������
		mov	a, m
		inr	a
		jz	mainLoop

		; ?
		call selFileToCmdLine

EnterCmdLine:	; ������� ������
		COLOR(COLOR_CMDSCREEN)
		mvi	 c, 1Fh
		call printChar

		; �������� � ����� ��� ������ 0Dh
		lhld v_cmdLineEnd
		mvi	 m, 0Dh

		; ��������� ���������
		call saveState

		; ��������� ����
		lxi  h, v_cmdLine
		jmp  fileExec
