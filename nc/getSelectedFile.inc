;----------------------------------------------------------------------------
; MXOS NC.COM
; �������� ���������� � ��������� ����� (FILE_INFO)
;
; �����:
;   hl - ����� ��������� FILE_INFO
;
; 2013-12-18 ����������������� vinxru
;----------------------------------------------------------------------------

getSelectedFile:
        push    de

        ; � ������ ����� ���������� ������ ������ ������
        ld      hl, state.panelA_firstFile
        call    readBytePanel
        ld      e, a

        ; ����� �������� �����
        ld      hl, state.panelA_curFile
        call    readBytePanel
        sub     e

        ; ����� ��������� �������� ����� � ������:
        ; hl = FILE_LIST_BUFFER + FILE_INFO_SIZE * curFile[state.activePanel];

        ld      hl, FILE_INFO_SIZE
        ld      e, a
        ld      d, 0
        call    bios_mul_hl_de

        ld      de, FILE_LIST_BUFFER
        add     hl, de

        pop     de
        ret

