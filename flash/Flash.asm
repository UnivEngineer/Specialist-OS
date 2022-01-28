;----------------------------------------------------------------------------
; MXOS
; FLASH.COM - ������� ����-����� �� AT29C040A
;
; ������� ����� ��� ������� (����� ��������� � �������� �):
; 1 - �������� ������� (256 ����, ����� �������� � �������� D, ����� ������ � HL);
; 2 - �������  ������� (256 ����, ����� �������� � �������� D, ����� ������ � HL);
; 3 - ������ ������ ����� (� ���������, � �������� �).
;
; �������������� ����-������ AT29C040A
;
; 2022-01-25 ����������� SpaceEngineer
;
;----------------------------------------------------------------------------

; ������� �������
fileGetSetDrive	= 0C842h	; ��������/���������� �������� ����������
installDriver   = 0C860h	; ���������� ������� ����������

; ���������� �������
v_flashPage     = 8FF9h     ; ������� �������� ����-����� (����. 1Fh = 31)

; ����� ��55 #2
IO_PROG_A    = 0FFE4h
IO_PROG_B    = 0FFE5h
IO_PROG_C    = 0FFE6h
IO_PROG_MODE = 0FFE7h

; ������� ��� �������� ������� ��55
LATCH_0 = 0Ah   ; ��� ������� ��22 = 0
LATCH_1 = 0Bh   ; ��� ������� ��22 = 1
WRITE_0 = 0Ch   ; ��� ������ = 0
WRITE_1 = 0Dh   ; ��� ������ = 1
READ_0  = 0Eh   ; ��� ������ = 0
READ_1  = 0Fh   ; ��� ������ = 1

; ����� ����������� ����� ��55
MASK_STANDBY  = 0C0h    ; ��� ������� ��22 = 0, ��� ������ = 1, ��� ������ = 1
MASK_READ     = 040h    ; ��� ������� ��22 = 0, ��� ������ = 1, ��� ������ = 0

; ����������
DRIVE = 7 ; "H"

; ����� ����-�����:
; ���� A - ������
; ���� B - ����� [A0-A7] ��� [A8-A15]
; ���� C0...C4 - ����� [A17-A20]
; ���� �5 - ����� ������� ������ [A8-A15] � ��22 (���. 1)
; ���� �6 - ����� ������ � ���  (���. 0)
; ���� �7 - ����� ������ �� ��� (���. 0)

;----------------------------------------------------------------------------

.ORG 0FA00H

    ; ���������� ������� ��� ���������� 7 ("H")
	MVI  A, DRIVE
	LXI  H, Driver
	JMP	 installDriver

    ; ���������� ���������� 7 ("H")
	MVI	 E, 1 ; ������� 1 - ���������� ����������
	MVI	 A, DRIVE
	JMP	 fileGetSetDrive

Driver:
	PUSH H
	PUSH D
	PUSH B

    MOV	 A, E   ; ����� �������
	CPI	 1
    JZ   FuncWrite
	CPI	 2
    JZ   FuncRead
	CPI	 3
	JZ	 FuncGetSize

Exit:
    PUSH PSW
    MVI  A, MASK_STANDBY    ; ��� ������� ��22 = 0, ��� ������ = 1, ��� ������ = 1
	STA	 IO_PROG_C
	POP  PSW
	POP  B
	POP  D
	POP  H
	RET

;----------------------------------------------------------------------------
; ������ ��������
; ����:
; D  - ����� ��������
; HL - ����� ������ � ������

FuncRead:
    ; ��������� ������ ��55
	MVI	A, 90H          ; ���� A - ����, ����� B � C - ����� 
	STA	IO_PROG_MODE

    ; ������ ������ 64� �������� � ����������� ����� � ���� C
    LDA v_flashPage     ; ����� 64� ��������, ���� 1Fh
    ANI 01FH            ; �� ������ ������
    ORI MASK_READ       ; ��� ������� ��22 = 0, ��� ������ = 1, ��� ������ = 0
	STA	IO_PROG_C       ; ���������� ����� 64� �������� � ����������� ����

    CALL LatchHiAddr    ; ����������� ������� ���� ������ (����� ��������) � ��22
	XRA  A
	MOV  E, A           ; �������� � 0 ������ ��������

ReadLoop:
    MOV  A, E
    STA  IO_PROG_B      ; ���� B = ������� ���� ������ � ��������
	LDA  IO_PROG_A      ; ������ ���� �� ����� A
	MOV	 M, A           ; ��������� � ������
	INX	 H
	INR	 E
	JNZ  ReadLoop
	JMP  Exit

;----------------------------------------------------------------------------
; ������ ��������
; ����:
; D  - ����� ��������
; HL - ����� ������ � ������

FuncWrite:
    ; ��������� ������ ��55
	MVI	A, 80H          ; ����� A, B � C - ����� 
	STA	IO_PROG_MODE

    ; ���������� ����� ���� �� ����-�����
    LDA v_flashPage     ; ����� 64� ��������, ���� 1Fh
    ANI 18h             ; �������� ��� ���� ����� 3-�� � 4-��
    MOV B, A            ; � B �������� ����� ���� * 8 (0, 8, 16, 24)

    ; ���������� ����������� ������ �� ������
    CALL DisableWriteProtection

    ; ������ ������ 64� �������� � ����������� ����� � ���� C
    LDA v_flashPage     ; ����� 64� ��������, ���� 1Fh
    ANI 01FH            ; �� ������ ������
    ORI MASK_STANDBY    ; ��� ������� ��22 = 0, ��� ������ = 1, ��� ������ = 1
	STA	IO_PROG_C       ; ���������� ����� 64� �������� � ����������� ����

    CALL LatchHiAddr    ; ����������� ������� ���� ������ (����� ��������) � ��22
	XRA  A
	MOV  E, A           ; �������� � ������ �������� (E = 0)

WriteLoop:
    MOV  A, E
    STA  IO_PROG_B      ; ���� B = ������� ���� ������ � ��������
	MOV	 A, M           ; ������ ���� �� ������
	STA  IO_PROG_A      ; ���������� � ���� A
    MVI  A, WRITE_0
    STA  IO_PROG_MODE   ; ������������� ��� ������ (C6 = 0)
    MVI  A, WRITE_1
    STA  IO_PROG_MODE   ; ������� ��� ������ (C6 = 1)
	INX	 H
	INR	 E
	JNZ  WriteLoop

    ; �������� ��������� ����������� ����� ���������������� ����-������
    DCX  H              ; ��������� ����� � ������
    MOV  C, M           ; ������ � C ��������� ���������� ���� �� ������
    DCR  E              ; ��������� ���� � ��������

    ; ��������� ������ ��55
    MVI  A, 90h        ; ���� A - ����, ����� B � C - ����� 
    STA  IO_PROG_MODE

    ; �������� �������� ����� ��������: ���� 20 ��, 
    ; 1 ���� = 197 ������ = 98.5 ���,
    ; 20 �� = 203 ��������
    MVI  L, 204

    ; ���� ��������
Wait:
    DCR  L
    JZ   Exit

    CALL ReadByteFromChip   ; ������ ��������� ���������� ���� �� ���� ������

    ; ���� ���������������� �� ��������, ���� ������ ����� ��������
    ; ��� 7 ���������� ����������� ����� � ��������� ����
    CMP  C
    JNZ  Wait

    JMP  Exit

;    ; �������� 840 ������ �� 24 ������ = 10 ��
;    lxi h, 840
;Delay:
;    dcx	h       ; 5 ������
;    mov	a, h    ; 5 ������
;    ora	l       ; 4 �����
;    jnz	Delay   ; 10 ������
;    jmp Exit

;----------------------------------------------------------------------------
; ���������� ����������� ������ �� ������
; ����:
;  B - ����� ���� * 8 (0, 8, 16, 24)

DisableWriteProtection:
    ; ����������� ���� ������� � ������� 64� �������� ����,
    ; ������� ����� �������� ������ �� ����
    push d
    ; ���������� ���� AAh �� ������ 05555h � ��� ����-������
    mvi  a, 0AAh
    lxi  d, 5555h
    call WriteByteToChip
    ; ���������� ���� 55h �� ������ 02AAAh � ��� ����-������
    mvi  a, 55h
    lxi  d, 2AAAh
    call WriteByteToChip
    ; ���������� ���� A0h �� ������ 05555h � ��� ����-������
    mvi  a, 0A0h
    lxi  d, 5555h
    call WriteByteToChip
    pop  d
    ret

;-----------------------------------------------------------------------------------
; ����������� ������ ����������
; �����:
;  A - ���������� ���������

FuncGetSize:
    XRA  A  ; ������ ������� ���� ����� ������ 256 (= 0) ���������
    JMP  Exit

;    XRA	A
;    MOV	B, A ; ������� ������� ���������
;    MOV	D, A ; ������ ������� 0 (FAT)
;    MVI	E, 4 ; ������� � 4 �����
;    CALL LatchHiAddr
;Rep:
;    MOV  A, E
;    STA  IO_PROG_B    ; ���� B = ������� ���� ������ � ��������
;    LDA  IO_PROG_A    ; ������ ���� �� ����� A
;    CPI  0FFH
;    JZ   SectorIsFree
;    INR  B    ; ��������� ������� ������� ���������
;SectorIsFree:
;    INR  E    ; ��������� ���� � �������� FAT
;    JNZ  Rep  ; E ��������� ��� �������� ����� 0FFh
;    JMP  Exit

;----------------------------------------------------------------------------
; ������������ �������� ����� ������ � ��22 ����-�����
; ����:
;   D - ������� ���� ������ (����� ��������)

LatchHiAddr:
    MVI A, LATCH_1
    STA IO_PROG_MODE  ; ��������� ������� ��22 (C5 = 1)
    MOV A, D
    STA IO_PROG_B     ; ���� B = ������� ���� ������ (����� ��������)
    MVI A, LATCH_0
    STA IO_PROG_MODE  ; �������� ������� ��22 (C5 = 0), ���������� ������� ���� ������
    RET

;----------------------------------------------------------------------------
; ����� ����� �� ���� ���� ������
; ����:
;   DE - ����� � 64� ��������
;   B - ����� 64� ��������
; �����:
;   A - ���� ������
;   ��� ������ ������� �������� (C7 = 0)

ReadByteFromChip:
    CALL LatchHiAddr    ; ����������� ������� ���� ������ (����� ��������) � ��22
    MOV  A, E
    STA  IO_PROG_B      ; ���� B = ������� ���� ������
    MOV  A, B
    ORI  MASK_READ      ; ��� ������� ��22 = 0, ��� ������ = 1, ��� ������ = 0
	STA  IO_PROG_C      ; ���������� ����� 64� �������� � ����������� ����
    LDA  IO_PROG_A      ; ������ ���� �� ����� A
    RET

;----------------------------------------------------------------------------
; ������ ����� � ��� ���� ������
; ����:
;   DE - ����� � 64� ��������
;   B - ����� 64� ��������
;   A - ���� ������

WriteByteToChip:
    STA  IO_PROG_A      ; ���� A = ���� ������
    CALL LatchHiAddr    ; ����������� ������� ���� ������ (����� ��������) � ��22
    MOV  A, E
    STA  IO_PROG_B      ; ���� B = ������� ���� ������
    MOV  A, B
    ORI  MASK_STANDBY   ; ��� ������� ��22 = 0, ��� ������ = 1, ��� ������ = 1
	STA  IO_PROG_C      ; ���������� ����� 64� �������� � ����������� ����
    MVI  A, WRITE_0
    STA  IO_PROG_MODE   ; ������������� ��� ������ (C6 = 0)
    MVI  A, WRITE_1
    STA  IO_PROG_MODE   ; ������� ��� ������ (C6 = 1)
    RET

;----------------------------------------------------------------------------

WaitFlag: .db 0     ; ���� �������� ���������� �������������� ����������� �������

;----------------------------------------------------------------------------

.END