;----------------------------------------------------------------------------
; MXOS NC.COM
; ������� F3 - ���������� ���������� � ����������
;
; 2013-12-18 ����������������� vinxru
;----------------------------------------------------------------------------

but_F3:     ; ��������� ����� ��������������� ������
            ld    a, (state.activePanel)
            push  af
            xor   1
            ld    (state.activePanel),a

            ; ������ ��������� ������
            ld    hl, state.panelA_info
            call  readBytePanel
            xor   1
            call  writeBytePanel

            ; �������������� �������� ������
            pop   af
            ld    (state.activePanel),a
            and   a
            jp z, loc_D701

            ; �������������� ������
            call  loadAndPrintA

            jp    saveStateAndReload

; ---------------------------------------------------------------------------

loc_D701:   call  loadAndPrintB
            jp    saveStateAndReload

