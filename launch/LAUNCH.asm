;+---------------------------------------------------------------------------
; MXOS
; ������ RKS �����
;
; 2021-01-27 ����������� SpaceEngineer
;----------------------------------------------------------------------------

; ����� �������� ��������
MON_ADDR = 0C000h

; ����� ���������� ������ �������� ��������
MON_ADDR_TEMP = 0D000h

; ������� �������
getch			= 0C803h ; �������� ����� � ����������
printChar		= 0C809h ; ����� ������� �� �����
printString 	= 0C818h ; ������� ������ �� �����
fileLoad		= 0C848h ; ��������� ���� �� ������ �� ��������� ����� �����
fileNamePrepare	= 0C85Ah ; ������������� ��� ����� �� ���������� ������
fileLoad2		= 0C866h ; ��������� ���� �� ������ DE

.org 0F100h

        ; � DE ��������� ����� ������ ����������
        push d
        lxi  h, aLoading
        call printString
		pop  h
        push h
        call printString
        pop  h

        ; �������� ���������
        ; ���������� ����� ����� � ������������ ����������
		lxi  d, nameBuffer
		call fileNamePrepare   ; HL = ��� ����� = ������ ����������
        xchg

		; ��������� ����
		lxi  h, nameBuffer
		call fileLoad
		jc   fileNotFoundRet

		; �������� ����� �������� (= ����� �������) ����� � DE
		lxi	d, 10
		dad	d   ; HL += 10
		mov	e, m
		inx	h
		mov	d, m

        ; �������� � ���� ��� �������� ��������
        push d

        lxi  h, aLoading
        call printString
		lxi  h, aMonitorPath
        call printString

        ; �������� ��������
        ; ���������� ����� ����� A:MON2.SYS
		lxi  h, aMonitorPath
		lxi  d, nameBuffer
		call fileNamePrepare

        ; ��������� ���� MON2.SYS �� 0D000h - �� ������ NC.COM
		lxi  h, nameBuffer
		lxi  d, MON_ADDR_TEMP ; �������� ����� �������� ����� �� DE
		call fileLoad2  ; ����� ������������ �������! BIOS 4.50 � ������ 
		jc   popFileNotFoundRet

		; �������� ������ ����� �������� � DE
		lxi	d, 12
		dad	d   ; HL += 12
		mov	e, m
		inx	h
		mov	d, m

        ; HL = MON_ADDR_TEMP
        ; DE = MON_ADDR_TEMP + ������ ��������
		lxi  h, MON_ADDR_TEMP
        xchg
		dad  d
        xchg

        ; �������� ������� �� ����� C000h. ��� ���� �� ����� BIOS, � ��������
        ; �������� �� ������ ����������. ������� � ����� ��� ��������� �����.
		lxi  b, MON_ADDR
        call memcpy

        ; ������ ��������. ������� ��� ��������� � ����� STD, ����������������,
        ; � ��������� ��������� �� ������ �� ������� �����.
        jmp  MON_ADDR

memcpy: ; ������������ �� HL � BC � ����������� �������, ���� HL �� ����� DE
        mov  a, m
        stax b
        call cmp_hl_de
        rz
        inx  h
        inx  b
        jmp  memcpy

cmp_hl_de:
        mov	a, l
        cmp	e
        rnz
        mov	a, h
        cmp	d
        ret

popFileNotFoundRet:
        pop  d ; ��������������� ���� ��� ����������� ���������� ������
fileNotFoundRet:
        lxi  h, aFileNotFound
        jmp  printString

        ; Esc + ) �������� KOI-8 �� ����� ��������� ������
aLoading:
        .db 0Ah,1Bh,"(Loading ",0

aFileNotFound:
        .db 0Ah,1Bh,"(File not found",0

aMonitorPath:
        .db "A:MON2.SYS",0

nameBuffer:
        .block 10

.end
