;----------------------------------------------------------------------------
; MXOS NC.COM
; ����� ������ ������ �� ������
;
; 2013-12-18 ����������������� vinxru
;----------------------------------------------------------------------------

;----------------------------------------------------------------------------
; ����������� ���� ������

redrawFilePanel:
        COLOR   COLOR_BORDER

        ; ��������� ������
        ld      hl, g_filePanel
        call    draw

        ; ����� ����� �����
        COLOR   COLOR_INFOLINE
        ld      hl, state.panelA_drive
        call    readBytePanel
        ld      hl, (P_DRIVE_LETTER_X << 8) + P_DRIVE_LETTER_Y
        call    setCursorPosPanel
        call    printDrive

        ; ����� NAME
    IF FULL_PANELS==0
        COLOR   COLOR_PANELNAME
        ld      hl, (P_NAME_X1 << 8) + P_NAME_Y
        call    setCursorPosPanel
        ld      hl, aNameName
        call    bios_printString

        ld      hl, (P_NAME_X2 << 8) + P_NAME_Y
        call    setCursorPosPanel
        ld      hl, aNameName
        call    bios_printString
    ENDIF

        ; ������ ������ ������ ������ ����
        ld      b, 0

;----------------------------------------------------------------------------
; ����������� ������ ������
; ����:
;   b - ����������� (0 - ������ ����, 1 - ����� �����)

printFilePanel:
        ; ������ ������ ���� ��� ����� �����?
        ld      a, b
        or      a
        push    af

        ; �������� ������ ������
        ld      hl, state.panelA_drive
        call    readBytePanel   ; a = ����� ����������
        call    loadFiles

        ; ���� ��� ������ ������ ������
        COLOR   COLOR_FILE

        pop     af
        jp nz,  printFilesDownUp

;----------------------------------------------------------------------------
; ���� ������ ������ ����

printFilesUpDown:
        ; ��������� ������ ������ - �� ������� ����
        ld      hl, FILE_LIST_BUFFER

        ; ��������� ������� - �� ������� ����
        ld      de, (P_FILE_LIST_X1 << 8) + P_FILE_LIST_Y

pfud_Loop:
        ; ��������� ���� �� ���������� ����� ������ ������
        ; (������ ������ ����� = FF)
        ld      a, (hl)
        inc     a
        ret z

        ; ��������� ��������� �������
        ex      de, hl
        call    setCursorPosPanel
        ex      de, hl

        ; ������ ����� �����
        call    printFileNamOnPanel

        ; ��������� ����
        ld      bc, FILE_INFO_SIZE
        add     hl, bc

        ; ������ �� ��������� ������
        ld      a, e
        add     10
        ld      e, a

        ; ������ �� ��������� �������
        cp      P_FILE_LIST_Y_MAX
        jp c,   pfud_Loop
        ld      a, d
        ld      de, (P_FILE_LIST_X2 << 8) + P_FILE_LIST_Y
        cp      20
        jp c,   pfud_Loop

        ret

;----------------------------------------------------------------------------
; ���� ������ ����� �����

printFilesDownUp:
        ; ��������� ������ ������ - �� ��������� ����
        ld      hl, FILE_LIST_BUFFER + (FILE_LIST_SIZE-1) * FILE_INFO_SIZE

        ; ��������� ������� - �� ��������� ����
        ld      de, (P_FILE_LIST_X2 << 8) + P_FILE_LIST_Y_MAX - 10

pfdu_Loop:
        ; ��������� ��������� �������
        ex      de, hl
        call    setCursorPosPanel
        ex      de, hl

        ; ������ ����� �����
        call    printFileNamOnPanel

        ; ���������� ����
        push    bc
        ld      bc, -FILE_INFO_SIZE
        add     hl, bc
        pop     bc

        ; ������ �� ���������� ������
        ld      a, e
        sub     10
        ld      e, a

        ; ������ �� ���������� �������
        cp      P_FILE_LIST_Y
        jp nc,  pfdu_Loop
        ld      a, d
        ld      de, (P_FILE_LIST_X1 << 8) + P_FILE_LIST_Y_MAX - 10
        cp      20
        jp nc,  pfdu_Loop

        ret
