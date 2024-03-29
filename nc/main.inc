;----------------------------------------------------------------------------
; MXOS NC.COM
; �������� ����
;
; 2013-12-18 ����������������� vinxru
;----------------------------------------------------------------------------

mainReload: ; �������������� ��� ������
            ld      a, (state.activePanel)
            and     a
            jp      nz, mainReload_0
            call    loadAndPrintA
            call    loadAndPrintB
            jp      mainDriveChanged

mainReload_0:
            call    loadAndPrintB
            call    loadAndPrintA
            
mainDriveChanged:
            ; ����� ���������� � ��� ������
            COLOR   COLOR_CMDLINE
            ld      hl, 1F3h
            ld      (bios_vars.cursorY), hl
            ld      hl, state.panelA_drive
            call    readBytePanel
            call    printDrive

            ; ����� ���������� � �����
            call    printInfoLine

mainLoop:   ; ������ ������ ���������� �����
            COLOR   COLOR_CURSOR
            call    drawCursor

            ; ��������� ������� � ��� ������
            ld      hl, (vars.cmdLinePos)
            ld      (bios_vars.cursorY), hl

            ; ���� ������� �������
            COLOR   COLOR_CMDLINE
            call    bios_getch

            ; ������� ������ ���������� �����
            push    af
            COLOR   COLOR_FILE
            call    drawCursor
            pop     af

            ; ������ ������� �������
            ld      c, a
            jp c,   but_ctrl
            and     a     ; F1 (00h)
            jp z,   but_F1
            dec     a     ; F2 (01h)
            jp z,   but_F2
            dec     a     ; F3 (02h)
            jp z,   but_F3
            dec     a     ; F4 (03h)
            jp z,   but_F4
            dec     a     ; F5 (04h)
            jp z,   but_F5
            dec     a     ; F6 (05h)
            jp z,   but_F6
            dec     a     ; F7 (06h)
            jp z,   but_F7
            dec     a     ; F8 (07h)
            jp z,   but_F8
            dec     a     ; ������� ����� (08h)
            jp z,   but_Left
            dec     a     ; Tab/��� (09h)
            jp z,   but_Tab
            dec     a     ; End/�� (0Ah)
            jp z,   but_End
            sub     2     ; Home/������� �����-����� (0Ch)
            jp z,   but_Home
            dec     a     ; Enter/�� (0Dh)
            jp z,   but_Enter
            sub     0Bh   ; ������� ������ (18h)
            jp z,   but_Right
            dec     a     ; ������� ����� (19h)
            jp z,   but_Up
            dec     a     ; ������� ���� (1Ah)
            jp z,   but_Down
            dec     a     ; Esc/��2 (1Bh)
            jp z,   but_Esc
            add     1Bh
            cp      7Fh   ; Del/�� (7Fh)
            jp z,   but_Bkspc
            cp      8Ah   ; F9 (8Ah)
            jp z,   but_F9
mainLoop_e: cp      20h   ; ������ (20h)
            jp c,   mainLoop

            ; ����� �������� � ��� ������
            ld      hl, (vars.cmdLineEnd)
            ld      c, a
            ld      (hl), a

            ; ���� ��� ������ �����������, �������
            ld      a, vars.cmdLineCtrl & 0FFh
            cp      l
            jp z,   mainLoop

            ; ����������� �����
            inc     hl
            ld      (vars.cmdLineEnd), hl

            ; ������� ������ �� �����
            COLOR   COLOR_CMDLINE
            call    bios_printChar

mainLoop_savePos: ; ?
            ld      hl, (bios_vars.cursorY)
            ld      (vars.cmdLinePos), hl

            jp      mainLoop

; ---------------------------------------------------------------------------

but_Bkspc:  ; ���� ��� ������ ������, �������
            ld      hl, (vars.cmdLineEnd)
            ld      a, vars.cmdLine & 0FFh
            cp      l
            jp z,   mainLoop

            ; ������� ���� ������
            dec     hl
            ld      (vars.cmdLineEnd), hl
            
            ; ������� ������ �� ������
            COLOR   070h
            ld      hl, asc_DC17
            call    bios_printString

            jp    mainLoop_savePos

; ---------------------------------------------------------------------------

but_Esc:    call    clearCmdLine
            jp      mainLoop

; ---------------------------------------------------------------------------

but_ctrl:   cp    0Dh
            jp z, ctrlEnter
            jp    mainLoop_e

; ---------------------------------------------------------------------------

ctrlEnter:  ; ���� ���� �� ������, �����
            call    getSelectedFile
            ld      a, (hl)
            cp      0FFh
            jp z,   mainLoop

            call    selFileToCmdLine
            jp      mainLoop

