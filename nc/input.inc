;----------------------------------------------------------------------------
; MXOS NC.COM
; ���� ������
;
; 2013-12-18 ����������������� vinxru
;----------------------------------------------------------------------------

input:      ; ����� ������
            push    hl
            ld      hl, vars.input
            call    printStringInv
            pop     hl

            ; �������� ��������
            ld      a, 1
            ld      (bios_vars.inverse),a

input_loop: ; ���� �������
            call  bios_getch

            ; Enter ��� ESC
            cp      0Dh
            jp z,   input_exit
            cp      1Bh
            jp z,   input_exit

            ; ���� ��������� ������� ��� ������, �� ����������
            cp      21h
            jp c,   input_loop

            ; ��������� ��� ������
            ld      c, a

            ; �������� ������ � �����
            ld      (de),a

            ; ��������
            cp      7Fh
            jp z,   input_bkspc

            ; ���� ������������
            ld      a, (vars.input + P_INPUT_WIDTH) & 0FFh  ; a = ������� ���� ������ ��������� ����� ������
            cp      e
            jp z,   input_loop

            ; ����� ������� �� �����            
            call    bios_printChar

            ; ���������� ����� ������
            inc     de

            jp      input_loop

; ---------------------------------------------------------------------------

input_bkspc:
            ; ���� ��� ������ ������, �������
            ld      a, vars.input & 0FFh
            cp      e
            jp z,   input_loop

            ; ��������� ����� ������
            dec     e

            ; ������� ������ � ������
            push    hl
            ld      hl, asc_DC17
            call    bios_printString
            pop     hl

            jp      input_loop

; ---------------------------------------------------------------------------

input_exit:
            ; ��������� ESC ��� Enter ��� ������ 
            ld      c, a

            ; ���������� � ����� ������ 0Dh, 0
            ex      de, hl
            ld      (hl), 0Dh
            inc     hl
            xor     a
            ld      (hl), a

            ; ��������������� ��������
            ld      (bios_vars.inverse),a

            ; �����           
            ld      a, c
            ret

