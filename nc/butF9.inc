;----------------------------------------------------------------------------
; MXOS NC.COM
; ������� F9 - ���������� ����� �� ����������
;
; 2013-12-18 ����������������� vinxru
;----------------------------------------------------------------------------

but_F9:     ; ������ ����
            call    drawWindowIfSel

            ; ������� "SAVING FROM A: TO TAPE"
            CENTER_LINE 20, 5Eh
            ld      hl, aSaveFromToTape
            call    bios_printString
            call    printSelDrive

            ; ?
            call    printInvSelFile

            ; ���� ������� Enter ��� ESC.
loc_D39C:   call    bios_keyScan
            cp      1Bh
            jp z,   mainReload
            cp      0Dh
            jp nz,  loc_D39C

            ; ���������� sp   � ��������� ���   ������.
            call    prepareTapeLoad

            call    drawWindowIfSel

            ; ������� "SAVING TO TAPE"
            CENTER_LINE 14, 5Eh
            ld      hl, aSavingToTape 
            call    bios_printString

            ; �������� ����� �� ����� 0
            call    loadSelFileAt0  ; �� ������ de - ������������ ����� ��������

            ; ��������� ������ �������� � ������� �����
            push    de  ; ������������ ����� ��������
            push    hl  ; ������ �����������
            ld      de, FILE_DESCRIPTOR.size
            add     hl, de
            ld      e, (hl)
            inc     hl
            ld      d, (hl) ; de = ������-1
            pop     hl  ; ������ �����������
            push    de  ; ������
            push    hl  ; ������ �����������
            ld      hl, 0   ; ���� �������� �� ����� 0
            add     hl, de  ; hl = ������-1 + 0
            ex      de, hl  ; de = ������-1 + 0
            ld      hl, 0

            ; ������ ����������� ����� �� hl �� de
            call    bios_calcCS ; bc = crc
            ld      h, b
            ld      l, c
            ld      (vars.tapeSaveCRC), hl

            ; �����-���
            call    tapeWritePilot  ; 200h �����, ����� E6h

            ; ������ �����
            ld      a, 0D9h
            call    bios_tapeWrite  ; ��� ���� 0D9h
            call    bios_tapeWrite
            call    bios_tapeWrite

            ; ��� �����
            pop     hl ; ������ ����������
            ld      bc, FILE_NAME_LENGTH+2 ; ����� (��������� +1); ������ ������ - 9 ��������, ����� - FILE_NAME_LENGTH+3
            call    tapeWriteBlock

            ; �����-���
            call    tapeWritePilot  ; 200h �����, ����� E6h

            pop     bc ; ������
            pop     hl ; ������������ ����� ��������
            call    tapeWriteWord

            add     hl, bc ; �������� ����� ��������
            call    tapeWriteWord

            ; ������ �����
            ld      hl, 0
            call    tapeWriteBlock    ; hl = �����, bc = ������-1

            ; ������ ����������� �����
            ld      hl, (vars.tapeSaveCRC)
            call    tapeWriteWord

            jp      mainReload

