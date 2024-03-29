;+---------------------------------------------------------------------------
; MXOS
; ���������� �������� �� ������ 0FFC0h � ������ NC.COM,
; � ����� ����������� ������ AUTOEX.BAT, ���� �� ������ �������.
;
; 2013-12-12 ����������������� vinxru
;----------------------------------------------------------------------------

reboot3:    ; ������������� �����
            ld      sp, STACK_ADDR

            ; ������������� ������� �������
            ld      b, 0
            call    setRAMDDriver

            ; ��������� ����������
            ld      hl, j_reboot3
            ld      (bios_vars.tapeError), hl

reboot3_0:
            jp      reboot3_1

; -------------------------------------
; ��� ���� ����� �������� ���� ���� ��� ����� �������� ������������.
; �� ���� ��� ������ ������������ �� �� ����������.

reboot3_1:
            ; ������ ���������� ���������� - ������ ��� ����� jp reboot3_2
            ld      hl, reboot3_2
            ld      (reboot3_0+1), hl

            ; ������������� ���������� �������
            ld      b, 1
            call    setRAMDDriver

            ; ���� � ���-����� �� ������ 0FF00h (��������� �������� ������)
            ; ��� ����������� �����, ������, �� ��������� ��������������
            ld      hl, 0FFBFh      ; ����� � ��������
            xor     a               ; 0 ��������
            call    bios_RAMDRead
            ld      a, c            ; ��������� ���� � c
            cp      5Ah
            jp z,   skipFormat      ; ����� ���� - ���������� ��������������

            ; ���������� �����
            xor     a               ; 0 ��������
            ld      c, 5Ah          ; ������������ ����
            call    bios_RAMDWrite

            ; ������������� ������� �������
            ld      b, 0
            call    setRAMDDriver

            ; ������ ����� "A:FORMAT.BAT"
            ld      hl, pathFormatBat
            call    fileExec

skipFormat:
            ; ���� ������ �������, ���������� ������ A:AUTOEX.BAT
            ld      a, (IO_KEYB_B)     
            and     2
            jp z,   reboot3_2

            ; ������ ����� A:AUTOEX.BAT
            ld      hl, pathAutoexecBat
            call    fileExec

; -------------------------------------

reboot3_2:
            ; ������ ����� A:NC.COM
            ld      hl, pathNcCom
            call    fileExec

            ; ������ ������������
            jp      j_reboot3

