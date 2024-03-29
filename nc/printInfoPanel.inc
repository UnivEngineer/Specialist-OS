;----------------------------------------------------------------------------
; MXOS NC.COM
; ��������� �������������� ������
;
; 2013-12-18 ����������������� vinxru
;----------------------------------------------------------------------------

printInfoPanel:
            COLOR   COLOR_BORDER

            ; ������ ������
            ld      hl, g_infoPanel
            call    draw

            COLOR   COLOR_INFOHEADER

            ; ������ "COMMANDER VERSION XXX"
            ld      hl, 1110h
            call    setCursorPosPanel
            ld      hl, aCommanderVer
            call    bios_printString

            ; ������ "(C) OMSK 1992"
            ;ld      hl, 1D1Ah
            ld      hl, 0E1Ah
            call    setCursorPosPanel
            ld      hl, aCopyright    
            call    bios_printString

            ; ��������� �������
            ld      hl, 0B30h
            call    setCursorPosPanel

            ; ����� ����� ���
            ld      de, 65504   ; ����������-MX/MX2

            ; ������ "XXXX BYTES MEMORY"
            COLOR   COLOR_INFONUMBER
            ld      b, 20h
            call    bios_printDecWord
            COLOR   COLOR_INFOTEXT
            ld      hl, aKBytesMemory
            call    bios_printString

            ; ��������� �������
            ld      hl, 0B3Ch
            call    setCursorPosPanel

            ; ��������� ����� ���
            call    bios_getMemTop
            inc     hl  ; ������� �� 1 ���� ������, ��� memTop
            ex      hl, de

            ; ������ "XXXX BYTES FREE"
            COLOR   COLOR_INFONUMBER
            ld      b, 20h
            call    bios_printDecWord
            COLOR   COLOR_INFOTEXT
            ld      hl, aKBytesFree
            call    bios_printString

            ; ��������� �������
            ld      hl, 0548h
            call    setCursorPosPanel

            ; ����� ����� ���� � ��
            ld      a, (bios_vars.ramPageCount)
            ; hl = a * 256
            ld      h, a
            ld      l, 0
            ; hl = hl / 4
            call    bios_div_hl_2
            call    bios_div_hl_2
            ex      hl, de

            ; ������ "XXXX KB EXTENDED MEMORY"
            COLOR   COLOR_INFONUMBER
            ld      b, 20h
            call    bios_printDecWord
            COLOR   COLOR_INFOTEXT
            ld      hl, aKBytesExtMemory
            call    bios_printString

            ; �������� ������ �����
            ld      b, 3    ; �����
            call    bios_diskDriver             ; de = ���������� ��������� �� �����, hl = ����� DISK_INFO
            ld      (vars.diskInfoPtr), hl   ; c������� ����� ��������� DISK_INFO

            ; ���������, ��������� �� ����
            ld      bc, DISK_INFO.isValid
            add     hl, bc                  ; hl = ����� ���� isValid
            ld      a, (hl)                 ; a = isValid
            cp      DISK_VALID              ; ���� �������� ������, ��� DISK_VALID (2)
            jp c,   pip_printDiskError      ; �� ��������� ����

            ;---------------------------------------------------------

pip_printDiskInfo:
            ; ��������� �������
            ld      hl, 055Ch
            call    setCursorPosPanel

            ; ������� ����� � ����������
            ; de = de * 256 / 1024 = hl / 4
            ex      hl, de
            call    bios_div_hl_2
            call    bios_div_hl_2
            ex      hl, de
            push    de  ; � ����� - ����� ����� � ��

            ; ������ "XXXX KBYTES TOTAL ON DRIVE A:"
            COLOR   COLOR_INFONUMBER
            ld      b, 20h
            call    bios_printDecWord
            COLOR   COLOR_INFOTEXT
            ld      hl, aKBytesTotalOnDrv
            call    bios_printString
            COLOR   COLOR_INFONUMBER
            call    printCurDrive
            COLOR   COLOR_INFOTEXT

            ; ��������� �������
            ld      hl, 0B98h
            call    setCursorPosPanel

            ; ������ "Volume label: XXXXXXXXXXX"
            COLOR   COLOR_INFOTEXT
            ld      hl, aVolumeLabel
            call    bios_printString
            COLOR   COLOR_INFONUMBER
            ; hl = ����� ���� v_diskInfo.volumeLabel
            ld      hl, (vars.diskInfoPtr)
            ld      de, DISK_INFO.volumeLabel
            add     hl, de
            call    bios_printString

            ; ��������� �������
            ld      hl, 0568h
            call    setCursorPosPanel
            
            ; ������� ���������� ����� �� �����.
            ; ��� �/� ����� �������� �������� �����,
            ; � ����������� �� ������� fat �� �����
            ; ���������: de = ���������� ��������� ���������
            call    bios_getDriveFreeSpace

            ; ������� ����� � ����������
            ; hl = hl * 256 / 1024 = hl / 4
            ex      hl, de
            call    bios_div_hl_2
            call    bios_div_hl_2
            ex      hl, de
            push    de  ; � ����� - ����� ���������� ����� � ��

            ; ������ "XXXX KBYTES FREE ON DRIVE A:"
            COLOR   COLOR_INFONUMBER
            ld      b, 20h
            call    bios_printDecWord
            COLOR   COLOR_INFOTEXT
            ld      hl, aKBytesFreeOnDrv
            call    bios_printString
            COLOR   COLOR_INFONUMBER
            call    printCurDrive

            ; ��������� �������
            ld      hl, 0574h
            call    setCursorPosPanel

            ; de = ���-�� ������          
            ld      hl, state.panelA_filesCnt
            ld      a, (state.activePanel)
            and     a
            jp nz,  pip_filesPresent
            inc     hl
pip_filesPresent:
            ld      e, (hl)
            ld      d, 0

            ; ������ "XX FILES USE"
            COLOR   COLOR_INFONUMBER
            ld      b, 20h
            call    bios_printDecWord
            COLOR   COLOR_INFOTEXT
            ld      hl, aFilesUse
            call    bios_printString

            pop     de  ; �� ����� - ����� ���������� ����� � ��
            pop     hl  ; �� ����� - ����� ����� � ��
            call    bios_sub_hl_de
            ex      hl, de

            ; ������ "XXXX BYTES IN"
            COLOR   COLOR_INFONUMBER
            ld      b, 0    ; �� ��������� ���������
            call    bios_printDecWord
            COLOR   COLOR_INFOTEXT
            ld      hl, aKBytesIn
            call    bios_printString

            ; ��������� �������
            ld      hl, 2C80h
            call    setCursorPosPanel

            ; ������ "A:"
            ; ����� ����� ���������� ���� � ������� �����...
            COLOR   COLOR_INFONUMBER
            jp      printCurDrive

            ; ^^^ �����

            ;---------------------------------------------------------

pip_printDiskError:
            push    af  ; � a - ��� 0 (��� ��������) ��� 1 (���� �� ��������������)

            ; ��������� �������
            ld      hl, 0B5Ch
            call    setCursorPosPanel

            ; ������ "DRIVE A:"
            COLOR   COLOR_INFOTEXT
            ld      hl, aDrive
            call    bios_printString
            COLOR   COLOR_INFONUMBER
            call    printCurDrive
            COLOR   COLOR_INFOTEXT

            ; ������ " HAS NO DRIVER" ��� " IS NOT FORMATTED"
            pop     af
            or      a
            jp z,   pip_printDiskNoDriver
            ld      hl, aNotFormatted
            jp      bios_printString

            ; ^^^ �����

pip_printDiskNoDriver:
            ld      hl, aHasNoDriver
            jp      bios_printString

            ; ^^^ �����
