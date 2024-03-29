;+---------------------------------------------------------------------------
; MXOS
; ��������� BAT ����. ���������� �������� fileExec
;
; 2013-12-12 ����������������� vinxru
;----------------------------------------------------------------------------

execBat:
            pop     de

            ; ������������� ����� �������� BAT-����� �� BAT_BUFFER (= FC00h)
            ld      hl, BAT_BUFFER
            ld      (v_batPtr), hl
            ex      de, hl

            ; �������� ����� �������� BAT-�����
            ; TODO: ��� �� �������� � ���-������ (A: ��� ��������� ������). �������
            ; ������ ��������� BAT-���� � ����� A:, ���� ��� ����� �������� � ��������
            ; ��� �� ������ ��� FC00h (= BAT_BUFFER)
            ld      hl, v_curFileDescr
            ld      c, 1
            call    fileGetSetAddr
            jp c,   badCommand

            ; ��������� ��� BAT-�����
            ld      hl, v_curFileDescr      
            ld      de, v_curFileDescr + FILE_NAME_LENGTH + 3
            ld      bc, v_batFileDescr
            call    memmove_bc_hl

            ; ��������� ���� ���������� BAT-����
            ld      a, (v_drive)
            ld      (v_batDrive+1), a   ; ��������������� ����

            ; ���� ���� ����������� ��� ������ ������ BAT-�����
execBat_loop:

            ; ��������� ������� ���� � �������� b
            ld      a, (v_drive)
            ld      b, a

v_batDrive:
            ; ������� ���� ���������� BAT ����
            ld      a, 1 ; << ���� ������ 1 ������� ����, ���������� BAT ����
            call    fileSelectDrive

            ; ��������� BAT-���� � ������
            ld      hl, v_batFileDescr 
            call    fileLoad
            ld      hl, (v_batPtr)

            ; ��������� ������ ������
            ld      d, h
            ld      e, l

            ; ���� ����� ������
execBat_0:
            ld      a, (hl)       
            inc     hl

            ; ������ ����� ������, ��������� ����
            cp      0Dh         
            jp z,   execBat_1

            ; ����� �����
            cp      0FFh        
            jp nz,  execBat_0
            ret

; ---------------------------------------------------------------------------

execBat_1:
            ; ��������� ���������
            ld      (v_batPtr), hl  

            ; �������������� ��������� �����          
            ld      a, b         
            call    fileSelectDrive

            ; ��������� ����
            ex      de, hl             
            call    fileExec

            jp      execBat_loop

