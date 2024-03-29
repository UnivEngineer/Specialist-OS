;----------------------------------------------------------------------------
; MXOS NC.COM
; ����� ���������� � ����� ����� ����
;
; 2013-12-18 ����������������� vinxru
;----------------------------------------------------------------------------

printInfoLine:
        ; ���� ������ ���, �������
        ld      hl, state.panelA_filesCnt
        call    readBytePanel
        and     a
        ret z

        COLOR   COLOR_INFOLINE

        ld      hl, (P_FILE_NAME_X << 8) + P_FILE_NAME_Y
        call    setCursorPosPanel

        ; ld      c, '\'
        ; call    bios_printChar

        ; ������������� hl �� ��������� �����
        call    getSelectedFile
        push    hl

        ; �������� ��� ����� � ���������� ����
        ld      de, vars.tempFileDescr  ; �����
        call    compactName
        ld      hl, vars.tempFileDescr
        ld      b, FILE_NAME_LENGTH + 6 ; ���� ����� ������ (name.ext + ' ' + ����������)
        ld      c, 20                   ; ���������� - ������ ��� ������
        call    printString2

        ; ����� ����� �����, ���� ��� ��������, ���� ����������
        ; �������, ����� ������� ���������� �����. ����� ������
        ; printString2 � ��� � b ��� ��� ���������� ��������,
        ; ���������� �� FILE_NAME_LENGTH + 6
		call    printSpaces	; b - ���-�� ��������

        ; ������� ������
        ld      hl, (P_FILE_DATA_X << 8) + P_FILE_DATA_Y
        call    setCursorPosPanel

        ; ������������� hl �� ���� ������ ��������
        pop     hl
        push    hl
        ld      bc, FILE_INFO.loadAddress + 1 ; bc = ��������
        add     hl, bc

        ; ������ ������� � ������� ����� ������ ��������
        ; � ��� �� �������� �� � HEX �����
        ld      a, (hl)
        call    bios_printHexByte
        dec     hl
        ld      a, (hl)
        call    bios_printHexByte

        ; ������� ������
		LD 	    c, 18h  ; ��� ������� "������"
		call	bios_printChar

        ; ������������� hl �� ���� ������� �����
        pop     hl
        ld      bc, FILE_INFO.size ; bc = ��������
        add     hl, bc

        ; ������ ������ ����� � de � �������� � ���������� �����
        ld      e, (hl)
        inc     hl
        ld      d, (hl)
        inc     de          ; � ����������� ����� �������� ������-1
        ld      b, 20h
        jp      bios_printDecWord
