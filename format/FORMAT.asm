;+---------------------------------------------------------------------------
; MXOS
; FORMAT.COM
;
; 2022-01-14 ����������������� � ���������� SpaceEngineer
;
; ���������:
; - "����� �����" �� ��������� Y (������������� ��� ������)
;    �������� FORMAT.COM B: Y
;
;----------------------------------------------------------------------------

; ������� DOS
getch			= 0C803h	; �������� ����� � ����������
printChar		= 0C809h	; ����� ������� �� �����
printString		= 0C818h	; ����� ������ �� �����
fileGetSetDrive	= 0C842h	; ��������/���������� �������� ����������
diskDriver		= 0C863h	; ������� ���������� ����������

Buffer          = 0D100h	; �����

;----------------------------------------------------------------------------

.org 0D000h

		ldax    d		; � DE ��������� ����� ������ ����������
		cpi     20h
		jnc		ReadParams	; ������, ���� ���� ��������

		; ������ ����� ����� ��� ��������������
ChooseDrive:
		lxi     h, str_ChoseDrive
		call    printString		; ����� ��������� 'CHOOSE DRIVE: '
		call    getch			; �������� ������� �������
		mov     c, a
		call    printChar
		cpi     21h				; ��������� � ��������
		jc		Abort			; ����� � ��, ���� ������ ��� �����
		mov     b, a			; ��������� ����� ����� � B
		jmp     ConfirmRequest

ReadParams:
		mov     b, a			; ��������� ����� ����� � B

SearchLoop1:					; ����� ������� ������� � ������ ����������
		ldax    d
		cpi     21h
		jc		SearchLoop2
		inx     d
		jmp     SearchLoop1

SearchLoop2:					; ������� ����������� ��������
		ldax    d
		cpi     20h
		jnz		SearchLoopExit
		inx     d
		jmp		SearchLoop2

SearchLoopExit:		
		cpi     'Y'				; ���� ������ �������� 'Y', ������� � ��������������
		jz		Confirmed

		; ������������� ��������������
ConfirmRequest:
		mov     a, b
		sta     str_A_Y_N		; �������� 'A' � ������ ��������� �� �������� �����
		lxi     h, str_Format
		call    printString     ; ����� ��������� 'FORMAT <�����>: [Y/N]?'
		call    getch			; �������� ������� �������
		mov     c, a
		call    printChar
		cpi     'Y'				; ��������� � 'Y'
		jnz		Abort			; ����� � ��, ���� �� 'Y'
		
Confirmed:
		mov     a, b			; ������������ ����� ����� � A

		; ����� ����� � �������� A
Format:
		sui     41h				; ����� �����
		cpi     08h				; ������������ ����� ����� = 7
		jnc     InvalidDrive	; �����, ���� �������� ����� �����
		mov     b, a			; ��������� ����� ����� � B

		; ���������� ��������� ���� �������
		mov     a, b	; ����� ����� � A
		mvi     e, 01h
		call    fileGetSetDrive

		; ������ ������ ����� � A
		mvi     e, 03h
		call    diskDriver
		mov     e, a	; ��������� ������ ����� � E
		dcr     a

		; ������� ������ (E ����)
		lxi     h, Buffer
ClearBufLoop:
		mvi     m, 0
		inr     l
		dcr     e
		jnz     ClearBufLoop

		; �������� ������ ��������� FAT (256 ����)
CreateFATLoop:
		inr     a
		jz      WriteToDisk
		mvi     m, 01h
		inr     l
		jmp     CreateFATLoop

		; ������ FAT �� ����
		; D - ����� �������
		; E - ��� ��������
WriteToDisk:
		lxi     d, 0001h  ; ������ ������� ����� 0
		call    diskDriver

		; �������� ������ ��������� �������� (256 ����)
CreateCatLoop:
		mvi     m, 0FFh
		inr     l
		jnz     CreateCatLoop

		; ������ �������� � ������ 3 �� 1
		mvi     d, 03h
WriteLoop:
		call    diskDriver
		dcr     d
		jnz     WriteLoop

		; ����� � ��
		ret

Abort:
		lxi     h, str_Aborting
		call    printString		; ����� ��������� 'ABORTING'
        ret

InvalidDrive:
		lxi     h, str_InvalidDrive
		call    printString		; ����� ��������� 'INVALID DRIVE LETTER'
        ret

;----------------------------------------------------------------------------
; ������

str_Format:
		.db 0Ah
		.text "FORMAT "

str_A_Y_N:
		.text "A: [Y/N]? "
		.db 0

str_ChoseDrive:
		.db 0Ah
		.text "CHOOSE DRIVE: "
		.db 0

str_InvalidDrive:
		.db 0Ah
		.text "INVALID DRIVE LETTER"
		.db 0

str_Aborting:
		.db 0Ah
		.text "ABORTING"
		.db 0

;----------------------------------------------------------------------------

.end
