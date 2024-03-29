;----------------------------------------------------------------------------
; MXOS NC.COM
; ����� ������� ��� ����������� � �������� �����
;
; 2013-12-18 ����������������� vinxru
;----------------------------------------------------------------------------

copyFileInt:
            ; �������� ���������� ����� � ������
            call  loadSelFileAt0
            push  de    ; ��� �������� �����

            ; �������������� ���    
            ex    de, hl
            ld    hl, vars.input
            call  bios_fileNamePrepare
            ex    de, hl

saveFileInt:
            ; hl = ����� ����������� �����
            push  hl

            ; ������������� ����� ������������ �����
            ld    de, FILE_DESCRIPTOR.loadAddress   ; �������� ���� ������ ������������ ������ �����������
            add   hl, de
            ld    de, 0     ; ���������� 0 � ���� loadAddress �����������
            ld    (hl), e
            inc   hl
            ld    (hl), d

            ; ��������� ����
            pop   hl
            call  bios_fileCreate

            ; �������� ����� �������� �����
            pop   de
            ld    c, 1
            call  bios_fileGetSetAddr

            ; �������, ���� ��� ������
            ret nc      

            ; ������ ����
            call  drawWindow2

            ; ����� "CAN'T CREATE FILE"
            COLOR   COLOR_DIALOG_ERR
            CENTER_LINE 18, 80h
            ld    hl, aCantCreateFile
            call  bios_printString

            ; ����� �����
            COLOR COLOR_DIALOG
            ld    hl, 5090h
            ld    (bios_vars.cursorY), hl
            ld    hl, vars.input
            call  printStringInv

anyKey:           ; ���� ������� ����� �������
            call  bios_keyScan
            inc   a
            jp z, anyKey

            ; �����
            pop   hl
            jp    mainReload

