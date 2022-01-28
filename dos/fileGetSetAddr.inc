;+---------------------------------------------------------------------------
; MXOS
; ���������/��������� ������ �������� �����
;
; �� ����� 
;   hl - ��� �����
;   �  - 0=������, 1=���������
;   de - �����
;
; �� ������ 
;   de - �����
;   bc,hl - �����������
;
; 2013-12-12 ����������������� vinxru
;----------------------------------------------------------------------------

fileGetSetAddr:	; ���� ���� � ������ � HL
		call	fileFind
		rc

		push	h

		; �������� ����� ��� ��������� ����� �����
		call	fileGetInfoAddr
		inx	h

		; ���������, ���� �� ����� � ������� C=0
		jnz	fileGetSetAd_0

		; �������� ����� ��������
		mov	m, e
		inx	h
		mov	m, d

		; ��������� ���������
		call	saveFatDir

		; �������
		pop	h
		ora	a
		ret

; ---------------------------------------------------------------------------

fileGetSetAd_0:	; ������ ����� ��������
		mov	e, m
		inx	h
		mov	d, m

		; �������
		pop	h
		ora	a
		ret
