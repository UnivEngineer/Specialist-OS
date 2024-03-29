;----------------------------------------------------------------------------
; MXOS NC.COM
; ������
;
; 2013-12-18 ����������������� vinxru
;----------------------------------------------------------------------------

Start:      ; ��������� ��������
            ld      hl, 0
            ld      (bios_vars.inverse), hl

            ; ��������� ���������. ���� ZF, ������, ��� ��������� NC.COM.
            ; ����� ��������� ���������.
            call    loadState
            jp z,   formatCalled

            ; ��������� ���������
            call    saveState

; ---------------------------------------------------------------------------

formatCalled:
            ; ������� ������ � ��������
            COLOR   COLOR_HELP_F
            ld      c, 1Fh  ; ������� ������
            call    bios_printChar
            ld      hl, 1FFh
            ld      (bios_vars.cursorY), hl
            ld      hl, aF1LeftF2RighF3         
            call    bios_printString

            ; ������������ ������ � ��������
            COLOR   COLOR_HELP_TEXT
            ld      hl, 92F7h           ; ����� ������� ��������������
            ld      d, 8                ; ���������� ���������������
colToolbarLoop:
            ld      bc, (4 << 8) + 9    ; b = ������ = 4 �����, c = ������ 9 ��������
            call    inverseRect
            inc     h
            inc     h
            dec     d
            jp nz,  colToolbarLoop

            ; ������� ��������� ������
            call    clearCmdLine

