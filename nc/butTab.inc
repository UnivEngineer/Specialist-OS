;----------------------------------------------------------------------------
; MXOS NC.COM
; ������� TAB - ����� �������� ������
;
; 2013-12-18 ����������������� vinxru
;----------------------------------------------------------------------------

but_Tab:    ; �������� ��������� ������
            ld      hl, state.panelA_info
            ld      a, (state.activePanel)
            and     a
            jp nz,  loc_D6C7
            inc     hl
loc_D6C7:   ld      c, a
            ld      a, (hl)

            ; ������ �� ������, �������
            and     a
            jp nz,  mainDriveChanged

            ; ������ �������� ������ �� ���������������
            ld      a, c
            xor     1                       
            ld      (state.activePanel),a

            ; ������������� ������ ������
            ld      hl, state.panelA_drive
            call    readBytePanel
            call    loadFiles   ; �� ����� ����� a=����������

            jp      saveStateAndReload

