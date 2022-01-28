j_reboot3:		jmp	reboot3			; C800: ��������� NC.COM
j_getch:		jmp	getch			; C803: �������� ����� � ����������
j_tapeRead:		jmp	tapeRead		; C806: ������ ����� � �����������
j_printChar:	jmp	printChar		; C809: ����� ������� �� �����
j_tapeWrite:	jmp	tapeWrite		; C80C: ������ ����� �� ����������
				jmp	input			; C80F: ���� ������ � ����������
				jmp	j_keyScan		; C812: �������� ��� ������� �������
				jmp	printHexByte	; C815: ������� 16-������ ����� (����)
j_printString:	jmp	printString1	; C818: ������� ������ �� �����
				jmp	j_keyScan		; C81B: �������� ��� ������� �������
				jmp	getCursorPos	; C81E: �������� ���������� ������� � HL (���������� � ��������)
				jmp	setCursorPos	; C821: ���������� ���������� ������� �� HL (���������� � ��������)
				jmp	tapeLoad		; C824: ��������� ��������� � �����������
				jmp	tapeSave		; C827: ��������� ��������� �� ����������
				jmp	calcCS			; C81A: ������ ����������� �����
				jmp	printHexWord	; C82D: ������� 16-������ ����� (�����)
				jmp	getMemTop		; C830: �������� ����� ��������� ������
				jmp	setMemTop		; C833: ���������� ����� ��������� ������
				jmp	printer			; C836: ���������� ���� �� ��������
#if RAMFOS_COMPATIBILITY
				jmp	strToHex        ; C839: ������������� ������ � HEX ������� � �����
#else
				jmp	j_reboot3		; C839: ��������� NC.COM
#endif
				jmp	reboot3			; C83C: ��������� NC.COM
				jmp	fileList		; C83F: �������� ������ ������
				jmp	fileGetSetDrive	; C842: ��������/���������� �������� ����������
				jmp	fileCreate		; C845: ������� ����
				jmp	fileLoad		; C848: ��������� ���� �� ������ �� ��������� ����� �����
				jmp	fileDelete		; C84B: ������� ����
				jmp	fileRename		; C84E: ������������� ����
				jmp	fileLoadInfo	; C851: ��������� ���������� � �����
				jmp	fileGetSetAddr	; C854: ��������/���������� ����� �������� �����
				jmp	fileGetSetAttr	; C857: ��������/���������� �������� �����
				jmp	fileNamePrepare	; C85A: ������������� ��� ����� �� ���������� ������
				jmp	fileExec		; C85D: ��������� ����
				jmp	installDriver	; C860: ���������� ������� ����������
j_diskDriver:	jmp	diskDriver		; C863: ������� ���������� �����
				jmp	fileLoad2		; C866: ��������� ���� �� ������ DE
				jmp	printChar_real	; C869: ����� ������� �� ����� (������ �������� �������)
