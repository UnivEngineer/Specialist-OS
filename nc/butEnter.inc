;----------------------------------------------------------------------------
; MXOS NC.COM
; ������� ENTER
;
; 2013-12-18 ����������������� vinxru
;----------------------------------------------------------------------------

but_Enter:  ; ���� ��� ������ �� ������
            ld      hl, (vars.cmdLineEnd)
            ld      a, vars.cmdLine & 0FFh
            cp      l
            jp nz,  EnterCmdLine

            ; ��������� ������� ���� � �������� b
            ld      e, 0
            call    bios_fileGetSetDrive
            ld      b, a

            ; ����������� ��� ����� A:NC.EXT
            ld      hl, aNcExt
            ld      de, vars.tempFileDescr; vars.tempFileDescr ������������ ��� �����
            call    bios_fileNamePrepare
            ex      de, hl

            ; ��������� ���� A:NC.EXT �� ������ 0
            ld      hl, vars.tempFileDescr
            ld      de, 0
            ld      c, 1
            call    bios_fileGetSetAddr ; �������� ����� �������� ����� �� de = 0
            call    bios_fileLoad
            jp c,   Enter_stdExec

            ; ����������� ���� �������
            ;mov    a, b
            ;mvi    e, 1
            ;call   bios_fileGetSetDrive

            ; �������� ��� ���������� �����
            call    getSelectedFile
            ld      a, (hl)
            inc     a
            jp z,   mainLoop ; ������ �� �������

            ; hl = ��������� �� ���������� �����
            ld      de, FILE_DESCRIPTOR.ext
            add     hl, de

            ld      de, 0

            ; ���������� � ���������� �� �����,
            ; ��������������� � ��������
Enter_extLoop:
            push    hl
            ld      c, 3
            call    bios_stricmp
            jp nz,  Enter_nextExt

            ; ������� ����������
            pop     hl
            inc     de

            ; ����������� ����� ���������� ����� � ��� ������
Enter_5:
            ld      hl, vars.cmdLine
Enter_4:
            ld      a, (de)
            ld      (hl),a
            cp      0Dh
            jp z,   Enter_3
            inc     a
            jp z,   Enter_3
            inc     hl
            inc     de
            jp      Enter_4

; ---------------------------------------------------------------------------

Enter_3:
            ld      (hl), ' '
            inc     hl
            ld      (hl), 0
            ld      (vars.cmdLineEnd), hl

            ; ����� ��������� ������ �� �����
            COLOR   COLOR_CMDSCREEN
            ld      hl, vars.cmdLine
            call    bios_printString

Enter_stdExec:
            ; �������� ��������� ����
            call     getSelectedFile

            ; ������ �� �������
            ld      a, (hl)
            inc     a
            jp z,   mainLoop

            ; ?
            call    selFileToCmdLine

EnterCmdLine:
            ; ������� ������
            COLOR   COLOR_CMDSCREEN
            ld      c, 1Fh
            call    bios_printChar

            ; �������� � ����� ��� ������ 0Dh
            ld      hl, (vars.cmdLineEnd)
            ld      (hl), 0Dh

            ; ��������� ���������
            call    saveState

            ; ��������� ����
            ld      hl, vars.cmdLine
            jp      bios_fileExec

; ---------------------------------------------------------------------------

Enter_nextExt:
            pop     hl

Enter_2:
            ld      a, (de)
            inc     de
            cp      0Dh
            jp z,   Enter_extLoop
            inc     a
            jp      nz, Enter_2
            jp      Enter_stdExec
