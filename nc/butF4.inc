;----------------------------------------------------------------------------
; MXOS NC.COM
; ������� F4 - ������ ���������
;
; 2013-12-18 ����������������� vinxru
;----------------------------------------------------------------------------

but_F4: ; ��������� ���-�� ������
        ld      hl, state.panelA_filesCnt
        call    readBytePanel

        ; ���� ������ ���, �������
        and     a
        jp z,   mainLoop

        ; ������ ���������
        ld      de, aEditor   ; "A:E.COM\r"
        jp      Enter_5
