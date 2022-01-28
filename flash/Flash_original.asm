;+---------------------------------------------------------------------------
; MXOS
; FLASH.COM - ������� ����-����� ��55
;
; ������� ����� ��� ������� (����� ��������� � �������� �):
; 1 - �������� ������ (256 ����, ����� ������� � �������� D, ����� ������ � HL);
; 2 - ������� ������  (256 ����, ����� ������� � �������� D, ����� ������ � HL);
; 3 - ������ ������ ����� (� ��������, � �������� �).
;
; 2022-01-24 ����������������� � ���������� SpaceEngineer
;
;----------------------------------------------------------------------------

; ������� DOS
fileGetSetDrive	= 0C842h	; ��������/���������� �������� ����������
installDriver   = 0C860h	; ���������� ������� ����������

; �����
IO_KEYB_MODE = 0FFE3h
IO_PROG_A    = 0FFE4h
IO_PROG_B    = 0FFE5h
IO_PROG_C    = 0FFE6h
IO_PROG_MODE = 0FFE7h

.ORG 0FA00H

    ; ���������� ������� ��� ����������� 7 ("H")
	MVI  A, 7
	LXI  H, Driver
	JMP	 installDriver

    ; ������� ���������� 7 ("H")
	MVI	 E, 1
	MVI	 A, 7
	JMP	 fileGetSetDrive

Driver:
    MOV	 A, E
	CPI	 1
	RZ          ; ������ �� ��������������
	PUSH H
	PUSH D
	PUSH B

    ; ��������� ����� ��55
	MVI	A, 90H
	STA	IO_PROG_MODE
	MVI	A, 0DH ; ??? ���� ����������
	STA	IO_KEYB_MODE

	MOV	A ,E
	CPI	3
	JZ	FuncGetSize
	CPI	2
	JNZ	Exit

    ; ������ �����
    ; ����:
    ; D  - ����� �����
    ; HL - ����� ������ � ������
FuncRead:
	XRA	 A
	MOV	 E, A
ReadLoop:
    CALL Read
	MOV	 M, A
	INX	 H
	INR	 E
	JZ   Exit
	JMP  ReadLoop

    ; ����������� ������ ����������
    ; �����:
    ; A - ���������� ��������
FuncGetSize:
    XRA	A
	MOV	B, A
	MOV	D, A
	MVI	E, 4 ; ��������� ���� � �������
LBL3:
    CALL Read
	CPI  0FFH
	JNZ  LBL4 ; ��������, ���� �� ����� �����
	INR  B    ; ��������� ������� ������
LBL4:
    INR  E    ; ��������� ���� � �������
	MOV  A, E
	CPI  0C0H
	JNZ  LBL3
	MVI  A, 0C0H
	SUB  B

    ; �������������� ������ � �����
Exit:
    PUSH PSW
	MVI  A, 0CH
	STA  IO_KEYB_MODE
	MVI  A, 9BH
	STA  IO_PROG_MODE
	POP  PSW
	POP  B
	POP  D
	POP  H
	RET

    ; ������ ������
    ; ����:
    ; DE - ����� � �����
    ; �����:
    ; A - ������
Read:
    XCHG
	SHLD IO_PROG_B
	LDA IO_PROG_A
	XCHG
	RET

.END
