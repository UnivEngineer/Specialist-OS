;----------------------------------------------------------------------------
; MXOS NC.COM
; ������� F5 - �����������
;
; 2013-12-18 ����������������� vinxru
;----------------------------------------------------------------------------

but_F5: ; ������ ����
        call    drawWindowIfSel

        ; ������ "COPYING FROM A: TO"
        ; ����� ������ ��� x = 60h
        ; �������� ������ ������ w = 15*3/2 = 22
        ; ���������� ������ x = 60h-22 = 4Ah
        CENTER_LINE 22, 5Eh
        ld      (bios_vars.cursorY), hl
        ld      hl, aCopyFromTo 
        call    bios_printString
        call    printSelDrive

        call    inputForCopyMove
        cp      1Bh
        jp z,   mainReload

        call    copyFileInt
        jp      mainReload
