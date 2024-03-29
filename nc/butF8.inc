;----------------------------------------------------------------------------
; MXOS NC.COM
; ������� F8 - �������� �����
;
; 2013-12-18 ����������������� vinxru
;----------------------------------------------------------------------------

but_F8:     ; ������ ����
            call    drawWindowIfSel

            ; ������� "DELETE FROM A:"
            CENTER_LINE 14, 5Eh
            ld          hl, aDeleteFrom
            call        bios_printString
            call        printSelDrive

            call        printInvSelFile

loc_D54C:   ; ���� ������� Enter ��� ESC
            call        bios_keyScan
            cp          1Bh
            jp z,       mainReload
            cp          0Dh
            jp nz,      loc_D54C

            ; ���� ���� ������ ��� ������
            call        getSelectedFile         
            ld          de, FILE_DESCRIPTOR.attrib
            add         hl, de
            ld          a, (hl)
            and         1
            jp z,       loc_D595

            call        drawWindow2

            ; ������� ����� "FILE IS READ-ONLY"
            COLOR       COLOR_DIALOG_ERR
            CENTER_LINE 18, 80h
            ld          hl, aFileIsReanOnly
            call        bios_printString

            ; ������� vars.input
            COLOR       COLOR_DIALOG
            ld          hl, 5090h
            ld          (bios_vars.cursorY), hl         
            ld          hl, vars.input
            call        printStringInv

loc_D581:   ; ���� �������� ������� ����� �������
            call        bios_keyScan
            inc         a
            jp nz,      loc_D581

loc_D588:   ; ���� �������� ������� Esc ��� Enter
            call        bios_keyScan
            cp          1Bh             ; Esc
            jp z,       mainReload      ; ���� Esc - �����
            cp          0Dh             ; Enter
            jp nz,      loc_D588        ; ���� Enter - �� ����� �������

loc_D595:   ; ������� ������� ����
            call        getSelectedFile
            call        bios_fileDelete

            ; ��������� ������� ������
            ld          hl, state.panelA_filesCnt
            call        readBytePanel
            dec         a
            call        writeBytePanel

            jp          mainReload

