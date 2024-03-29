;+---------------------------------------------------------------------------
; MXOS
; ������������ ������� ����������� ������� ����
; ������������ �� RAMFOS
;
; �� �����
;  �  = ������������ ����� ��������, ��������� MAX_ARAM_PAGE
;  hl = ����� � �������� ���� ��� ������ ��������� �����
;
; �� ������
;  bios_vars.ramPageCount = ������������ ����� ��������
;  NZ - ���� ������� (�������� ������������ ����������� ������� Ramfos)
;   Z - ���� �� �������
;
; 2013-11-01 ����������� vinxru
; 2022-02-02 ���������� SpaceEnigneer
;----------------------------------------------------------------------------

checkRAMD:  ; ������������� ���������� �������.
            ; ��������������� ������� ������� �� ����,
            ; �.�. �� ����� ���������� � reboot3
            ld      b, 1
            call    setRAMDDriver

            ; ���������� �� ��� �������� ���������� ������ �� ������� � �������
checkRAMD_0:
            ld      c, a        
            inc     c
            call    bios_RAMDWrite
            dec     a
            jp p,   checkRAMD_0

            ; ������ ������. ���� ����� �������� ���������, ������ ���� ����� ��������
            ; a = -1

checkRAMD_1:
            ; ������ ��������� ��������
            inc     a
            call    bios_RAMDRead

             ; ������ ���� ������� ����� �������� + 1, ���� ���, �� �������
            dec     c
            cp      c            
            jp nz,  checkRAMD_2

            ; ��������� ����� ��������� �������� + 1
            inc     a
            ld      (bios_vars.ramPageCount), a
            dec     a

            ; ��� ��������, �������
            cp      RAMD_MAX_PAGE          ; RAMD_MAX_PAGE �� ����� ���� �����!
            jp nz,  checkRAMD_1
             
checkRAMD_2:
            ; ���� ���� �������, ������� c ������ NZ
            or      a
            ret nz

            ; � ������ ������ ������� � ramPageCount ���� � ������� � ������ Z
            ld      (bios_vars.ramPageCount), a
            ret

