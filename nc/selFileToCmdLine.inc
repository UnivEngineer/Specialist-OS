;----------------------------------------------------------------------------
; MXOS NC.COM
; ������������ ������ ���� ��������� �����
;
; ����:
;   hl - ����� ����������� �����
;
; 2013-12-18 ����������������� vinxru
;----------------------------------------------------------------------------

getSelFilePath:
        ex      de, hl

        ; �������� ������� ����������
        ld      hl, state.panelA_drive
        call    readBytePanel
        add     'A'

        ; ������� ��� � ��� ������
        ld      hl, (vars.cmdLineEnd)
        ld      (hl), a
        inc     hl

        ; ������� ����������
        ld      (hl), ':'
        inc     hl

        ; ������� ��� �����
        ex      de, hl
        call    compactName

        ; ����������� �����
        dec     de
        ld      a, vars.cmdLineCtrl & 0FFh
        cp      e
        jp nc,  loc_D146
        ld      e, a
loc_D146:   
        ; ����������
        xor     a
        ld      (de), a
        ret

;----------------------------------------------------------------------------
; MXOS NC.COM
; ��������� ��������� ���� � ��� ������
;
; ����:
;   hl - ����� ����������� �����
;
; 2013-12-18 ����������������� vinxru
;----------------------------------------------------------------------------

selFileToCmdLine:
        call    getSelFilePath

        ; �����
        COLOR   COLOR_CMDLINE
        ld      hl, (vars.cmdLineEnd)
        call    bios_printString

        ; ���������� ��� �����
        ex      de, hl
        ld      (vars.cmdLineEnd), hl

        ; ��������� ����� ���������
        ld      hl, (bios_vars.cursorY)
        ld      (vars.cmdLinePos), hl
        ret

