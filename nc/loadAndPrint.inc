;----------------------------------------------------------------------------
; MXOS NC.COM
; ��������� ����� � ����������
;
; 2013-12-18 ����������������� vinxru
;----------------------------------------------------------------------------

loadAndPrintB:
            ; ������� ������ �������� ������ �� ������
            ld      a, (state.activePanel)
            push    af
            ld      a, 1
            ld      (state.activePanel),a

            ; ��������� � ������ ����
            ld      a, (state.panelB_info)
            jp      loadAndPrint

; ---------------------------------------------------------------------------

loadAndPrintA:
            ; ������� ������ �������� ������ �� �����
            ld      a, (state.activePanel)
            push    af
            ld      a, 0
            ld      (state.activePanel),a

            ; ������ ������?
            ld      a, (state.panelA_info)

loadAndPrint:
            and     a
            jp nz,  loadAndPrint_2

            ; ������� ������ ������
            call    redrawFilePanel

            jp      loadAndPrint_3

; ---------------------------------------------------------------------------

loadAndPrint_2:   ; ������ ������ ����������
            call    printInfoPanel

loadAndPrint_3:   ; ��������������� �������� ������
            pop     af
            ld      (state.activePanel),a

            ; ��������� ������ ������
            ld      hl, state.panelA_drive
            call    readBytePanel
            call    loadFiles   ; �� ����� a ������ ��������� ����� ����������
            ret

