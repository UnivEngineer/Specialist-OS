;----------------------------------------------------------------------------
; MXOS NC.COM
; ������� ����������� �������
;
; 2013-12-18 ����������������� vinxru
; 2022-02-16 ���������� SpaceEngineer
;----------------------------------------------------------------------------

; ---------------------------------------------------------------------------
; �� ���� ���� ������

but_Down:   ; c = ���������� ������
            ld      hl, state.panelA_filesCnt
            call    readBytePanel
            ld      c, a

            ; a = ����, �� ������� ������
            ld      hl, state.panelA_curFile
            call    readBytePanel

            ; ������ �� ��������� ����
            inc     a

            ; ���� ������ �� ��������� ����� + 1, �������
            cp      c
            jp z,   mainDriveChanged

            ; ����� ��������� ����� ��������� �������
            ; � �������� ������, ���� ����
            jp      scrollPanel

; ---------------------------------------------------------------------------
; �� ���� ���� �����

but_Up:     ; a = ����, �� ������� ������
            ld      hl, state.panelA_curFile
            call    readBytePanel

            ; ���� ������ �� ������� �����, �������
            or      a
            jp z,   mainDriveChanged

            ; ������ �� ���������� ����
            dec     a

            ; ����� ��������� ����� ��������� �������
            ; � �������� ������, ���� ����
            jp      scrollPanel

; ---------------------------------------------------------------------------
; �� FILE_LIST_SIZE/2 ������ ������

but_Right:  ; c = ���������� ������
            ld      hl, state.panelA_filesCnt
            call    readBytePanel
            ld      c, a
            dec     c

            ; a = ����, �� ������� ������
            ld      hl, state.panelA_curFile
            call    readBytePanel

            ; ����������� �� FILE_LIST_SIZE/2 ������, ���� �� ����� �� ����� ������
            add     FILE_LIST_SIZE/2
            cp      c

            ; ����� ������������� ������ �� ��������� ����
            jp c,   scrollPanel
            ld      a, c
            jp      scrollPanel

; ---------------------------------------------------------------------------
; �� FILE_LIST_SIZE/2 ������ �����

but_Left:   ; a = ����, �� ������� ������
            ld      hl, state.panelA_curFile
            call    readBytePanel

            ; ��������� �� FILE_LIST_SIZE/2, ���� �� ����� �� 0
            sub     FILE_LIST_SIZE/2

            ; ����� ������������� ������ �� ������� ����
            jp p,   scrollPanel
            xor     a
            jp      scrollPanel

; ---------------------------------------------------------------------------
; �� ������ ����

but_Home:   ld      hl, state.panelA_curFile
            xor     a
            jp      scrollPanel

; ---------------------------------------------------------------------------
; �� ��������� ����

but_End:    ld      hl, state.panelA_filesCnt
            call    readBytePanel
            dec     a
            ld      hl, state.panelA_curFile
            jp      scrollPanel

; ---------------------------------------------------------------------------
; ������ ������ ��������� ���������� � ��������� ������, ���� ����

scrollPanel:
            ; ������� ��������� panelA_curFile
            call    writeBytePanel

            ; c = ���������� ������
            ld      hl, state.panelA_filesCnt
            call    readBytePanel
            ld      c, a
            dec     c

            ; b = ����, � �������� ���������� ������ ������ ������
            ld      hl, state.panelA_firstFile
            call    readBytePanel
            ld      b, a

            ; a = ����, �� ������� ������
            ld      hl, state.panelA_curFile
            call    readBytePanel

            ; a = ��������� ������� ������������ ������ ������ ������ ������
            sub     b

            ; ���� ������ ���� ������� ����� ������, �������� ����
            jp c,   scrollDown

            ; ���� ������ ���� ���������� ����� ������, �������� �����
            sub     FILE_LIST_SIZE
            jp nc,  scrollUp

            ; ����� �������
            jp      mainDriveChanged

scrollDown: ; ��������� ������ ����
            add     a, b

            ; ���� ��������� ������, �������
            jp nc,  mainDriveChanged

            ; ����������� ����������� ������� ������ - ������ ����
            ld      b, 0
            jp      scrollExit

scrollUp:   ; ��������� ������ �����
            inc     b
            add     a, b

            ; ���� ��������� ������, �������
            cp      c       ; c = ���������� ������
            jp nc,  mainDriveChanged

            ; ����������� ����������� ������� ������ - ����� �����
            ld      b, 1

scrollExit: ; ��������� ����� �������� panelA_firstFile
            ld      hl, state.panelA_firstFile
            call    writeBytePanel

            ; �������������� ������ ������ ������
            ; b = �����������
            call    printFilePanel

            ; �������
            jp      mainDriveChanged

; ---------------------------------------------------------------------------
