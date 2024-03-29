;+---------------------------------------------------------------------------
; MXOS
; ����������� ����� ����� C800
;
; 2013-12-12 ����������������� vinxru
;---------------------------------------------------------------------------

j_reboot3:      jp  reboot3             ; C800: ��������� NC.COM
j_getch:        jp  getch               ; C803: �������� ����� � ����������
j_tapeRead:     jp  t_tapeRead          ; C806: ������ ����� � �����������
j_printChar:    jp  printChar           ; C809: ����� ������� �� �����
j_tapeWrite:    jp  t_tapeWrite         ; C80C: ������ ����� �� ����������
                jp  input               ; C80F: ���� ������ � ����������
                jp  j_keyScan           ; C812: �������� ��� ������� �������
                jp  printHexByte        ; C815: ������� 16-������ ����� (����)
j_printString:  jp  printString1        ; C818: ������� ������ �� �����
                jp  j_keyScan           ; C81B: �������� ��� ������� �������
                jp  getCursorPos        ; C81E: �������� ���������� ������� � hl (���������� � ��������)
                jp  setCursorPos        ; C821: ���������� ���������� ������� �� hl (���������� � ��������)
                jp  t_tapeLoad          ; C824: ��������� ��������� � �����������
                jp  t_tapeSave          ; C827: ��������� ��������� �� ����������
                jp  calcCS              ; C82A: ������ ����������� �����
                jp  printHexWord        ; C82D: ������� 16-������ ����� (�����)
                jp  getMemTop           ; C830: �������� ����� ��������� ������
                jp  setMemTop           ; C833: ���������� ����� ��������� ������
                jp  printer             ; C836: ���������� ���� �� ��������
    IF RAMFOS_COMPATIBILITY
                jp  strToHex            ; C839: [MXOS2] ������������� ������ � HEX ������� � �����
    ELSE
                jp  j_reboot3           ; C839: ��������� NC.COM
    ENDIF
                jp  reboot3             ; C83C: ��������� NC.COM
                jp  fileList            ; C83F: �������� ������ ������
                jp  fileGetSetDrive     ; C842: ��������/���������� �������� ����������
                jp  fileCreate          ; C845: ������� ����
                jp  fileLoad            ; C848: ��������� ���� �� ������ �� ��������� ����� �����
                jp  fileDelete          ; C84B: ������� ����
                jp  fileRename          ; C84E: ������������� ����
                jp  fileLoadInfo        ; C851: ��������� ���������� � �����
                jp  fileGetSetAddr      ; C854: ��������/���������� ����� �������� �����
                jp  fileGetSetAttr      ; C857: ��������/���������� �������� �����
                jp  fileNamePrepare     ; C85A: ������������� ��� ����� �� ���������� ������
                jp  fileExec            ; C85D: ��������� ����
                jp  installDriver       ; C860: ���������� ������� ����������
j_diskDriver:   jp  diskDriver          ; C863: ������� ���������� �����
                jp  fileLoad2           ; C866: ��������� ���� �� ������ de
                jp  printChar_real      ; C869: [MXOS2] ����� ������� �� ����� (������ �������� �������)
                jp  printDecWord        ; C86C: [MXOS2] ����� ����� ����� � ���������� �����
                jp  setRAMDDriver       ; C86F: [MXOS2] ���������� ������� ������� � ����
                jp  getDriveFreeSpace   ; C872: [MXOS2] �������� ���������� ��������� ����� �����
                jp  upperCase           ; C875: [MXOS2] ������� ���� ������� ���-8 � ������� �������
                jp  strcmp              ; C878: [MXOS2] ��������� �����, �������������� � ��������
                jp  stricmp             ; C87B: [MXOS2] ��������� �����, ���������������� � ��������
                jp  sub_hl_de           ; C87E: [MXOS2] ��������� hl � de: hl = hl - de
                jp  div_hl_2            ; C881: [MXOS2] ������� hl �� 2: hl = hl / 2
                jp  mul_hl_de           ; C884: [MXOS2] ��������� hl �� de: hl = hl * de
                jp  div_hl_de           ; C887: [MXOS2] ������� hl �� de: hl = hl / de, de = hl % de
