;----------------------------------------------------------------------------
; MXOS NC.COM
; ���� ������ � ������� ���������� ���������� ��� ���������� �����
;
; 2013-12-18 ����������������� vinxru
;----------------------------------------------------------------------------

inputForCopyMove:
            ; ��������� �������
            ld      hl, 3D6Eh
            ld      (bios_vars.cursorY), hl

            ; ������ ��� ��������������   
            ld      de, vars.input

            ; ������������ �� ������?
            ld      hl, state.panelA_info
            ld      a, (state.activePanel)
            and     a
            jp nz,      loc_D255
            inc     hl
loc_D255:   ld      a, (hl)

            ; ���� �� ������������, �� ���������� � ������ �� ��������
            and     a
            jp nz,      loc_D265

            ; ���������� ����� ����������
            inc     hl
            inc     hl
            ld      a, (hl)
            add     'A'
            ld      (de),a
            inc     de

            ; ���������� ':'
            ld      a, ':'
            ld      (de),a
            inc     de

loc_D265:   ; �������� ��������� ����
            call    getSelectedFile
            call    compactName

            ; �������� ����������
            dec     de  ; compactName ��������� � ����� ������ ' ', 0Dh
            dec     de
            xor     a
            ld      (de),a

            ; ������� � �������������
            jp      input

