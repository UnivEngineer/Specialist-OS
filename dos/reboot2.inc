;+---------------------------------------------------------------------------
; MXOS
; ������������ (�����������)
;
; 2013-12-12 ����������������� vinxru
;----------------------------------------------------------------------------

reboot2:

    IF BOOT_FROM_TAPE
            ; ���������, ������ �� ����� ������ �������
            call    j_keyScan
            push    af
    ENDIF

            ; ������� ������ �� �� �����
            ld      hl, txtBiosVer
            call    printString1

            ; ���������� ������ ����
            ld      hl, RAMD_PAGE_END
            ld      a, (bios_vars.ramPageCount)
            call    checkRAMD

            ; ������� ����� ���� �� �����
            ld      hl, txtRAM
            call    printString1

            ; ��������� ����� ���� � ������
            ld      a, (bios_vars.ramPageCount)
            ld      h, 0
            ld      l, a    ; hl = ����� � ���������
            inc     l       ; +1 �������� - �������� ������
            ld      de, 64
            call    mul_hl_de  ; hl = hl * 64
            ex      hl, de
            call    printDecWord

            ld      hl, txtKB
            call    printString1

        IF BOOT_FROM_TAPE==0

            ; ���������� �������� �������
            jp      j_reboot3

        ELSE

            ; ���� �� ������ �� ���� �������, ���������� �������� �������
            pop     af
            inc     a
            jp z,   j_reboot3

            ; ��������� ������� �����������
            ld      hl, aATape_com
            call    fileExec

            ; ��������� ��������� � ����� (��� �������� ��) � ���������
            ld      hl, txtLoadingFromTape
            call    printString1
            call    t_tapeLoadInt
            ld      hl, (bios_vars.tapeAddr)
            jp      (hl)

        ENDIF
