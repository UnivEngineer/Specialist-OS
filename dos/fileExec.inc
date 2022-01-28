;+---------------------------------------------------------------------------
; MXOS
; ��������� ����
;
; �� �����
;  hl - ��� ������ � ������� [����:]����[ ���������]
;
; �� ������
;  �f - ������
;
; 2013-12-12 ����������������� vinxru
;----------------------------------------------------------------------------

fileExec:	; ���� ����� ���������� ���������
		lxi d, v_cmdLine
		
		; ���� ������ ������ ������ �������, ������� c CF
		mov a, m
		cpi ' '
		jc  stc_ret

		; ������������ ������ ��� ������ (+2 �����������, ����� 82h)
		mvi c, 80h		

fileExec_0:	; ���� ������ ������ �������, �������
		mov a, m
		cpi 20h
		jc  fileExec_1

		; �������� ������� �� HL � DE
		stax d
		inx  h
		inx  d

		; ��������� 80h ���
		dcr c
		jnz fileExec_0

fileExec_1:	; � ����� ����� �������� 13,0
		mvi  a, 0Dh
		stax d
		inx  d
		xra  a
		stax d

		; ������� �� ����� 0Ah
		call printCharA

		; ����� ����� ����� �� �����
		lxi  h, v_cmdLine
		call j_printString

		; ���������� ����� �����
		lxi  h, v_cmdLine
		lxi  d, v_fileName
		call fileNamePrepare

        ; ���������� � ����� ������� ����� ��������� ������ (������ ������ ����������)
		push h

		; ���� ��� BAT ����
		lxi  d, aBat ; "BAT"
		call cmpFileExt
		jz   execBat

		; ���� ��� COM ��� EXE ����
		call cmpFileExt
		jz   execExeCom
		call cmpFileExt
		jnz  execError

execExeCom:
        ; ����������� �� ����� ������� ����� ��������� ������ (������ ������ ����������)
		pop	d

		; ��������� ����
		lxi  h, v_fileName
		call fileLoad
		jc   badCommand

        ; ����� ��������� ������
		push d

		; ������� �� ����� 0Ah
		;call	printCharA

		; �������� ����� �������� (= ����� �������) ����� � DE
		lxi	d, 10
		dad	d   ; HL += 10
		mov	e, m
		inx	h
		mov	d, m

		; ����� ��������
		lxi	h, defAppReturn
		xthl

		; ������ ��������� (������� �� DE)
		xchg    ; HL <--> DE
		pchl    ; PC <-- HL

; ---------------------------------------------------------------------------

defAppReturn:
		ora	a
		ret

; ---------------------------------------------------------------------------

execError:
        pop d
badCommand:
        lxi  h, aBadCommandOrFi ; "\nBAD COMMAND OR FILE NAME"
        call j_printString
stc_ret:
        stc
        ret

; ---------------------------------------------------------------------------

printCharA:
        mvi c, 0Ah
        jmp j_printChar
