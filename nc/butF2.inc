;----------------------------------------------------------------------------
; MXOS NC.COM
; ������� F1, F2. ����� ���������� ��� ������
;
; 2013-12-18 ����������������� vinxru
;----------------------------------------------------------------------------

but_F2:     ; �������� ������������� state.activePanel=1
            ld    a, (state.activePanel)
            push  af
            ld    a, 1
            ld    (state.activePanel), a

            ; ������
            call  chooseDrive

            ; ���� ������� ����������, �� ��������� ���
            cp    0FFh
            jp z, loc_D5C3          
            ld    (state.panelB_drive), a

            ; � ����� ������ ������ ������� � ���������� ������ � ������
            xor   a
            ld    (state.panelB_info), a
            ld    (state.panelB_curFile), a
            ld    (state.panelB_firstFile), a

loc_D5C3:   ; ��������������� state.activePanel
            pop   af
            ld    (state.activePanel), a

            ; ������������� � �������������� ������ ������
            call  loadAndPrintB

            ; ������������� � �������������� ������ ������, ������ ���� ��� � ������ ����������
            ld    a, (state.panelA_info)
            and   a
            jp z, saveStateAndReload
            call  loadAndPrintA

            ; ��������� ��������� � �������
            jp    saveStateAndReload

; ---------------------------------------------------------------------------

but_F1:     ; �������� ������������� state.activePanel=2
            ld    a, (state.activePanel)
            push  af
            ld    a, 0
            ld    (state.activePanel), a

            ; ������
            call  chooseDrive

            ; ���� ������� ����������, �� ��������� ���
            cp    0FFh
            jp z, loc_D5F2
            ld    (state.panelA_drive), a

            ; � ����� ������ ������ ������� � ���������� ������ � ������
            xor   a
            ld    (state.panelA_info), a
            ld    (state.panelA_curFile), a
            ld    (state.panelA_firstFile), a

loc_D5F2:   ; ��������������� state.activePanel
            pop   af
            ld    (state.activePanel), a

            ; ������������� � �������������� ������ ������
            call  loadAndPrintA

            ; ������������� � �������������� ������ ������, ������ ���� ��� � ������ ����������
            ld    a, (state.panelB_info)
            and   a
            jp z, saveStateAndReload
            call  loadAndPrintB
            
saveStateAndReload:
            ; ��������� ��������� � �������
            call  saveState
            jp    mainDriveChanged

; ---------------------------------------------------------------------------

chooseDrive:
            COLOR   COLOR_DIALOG

            ld    hl, g_chooseDrive
            call  draw

            ; ������� ����� "CHOOSE DRIVE:"
            ld    hl, 1E63h
            call  setCursorPosPanel
            ld    hl, aChooseDrive
            call  bios_printString

            ; ������� ����� "A   B   C   D"
            ld    hl, 1C74h
            call  setCursorPosPanel
            ld    hl, aABCD
            call  bios_printString

            ; ������� ����� "E   F   G   H"
            ld    hl, 1C7Fh
            call  setCursorPosPanel
            ld    hl, aEFGH   
            call  bios_printString

            ; ������ ��� �������
            ld    a, (bios_vars.cursorCfg)
            push  af
            xor   a
            ld    (bios_vars.cursorCfg), a

            ; ��������� ������� ����������
            ld    hl, state.panelA_drive
            call  readBytePanel
            ld    (vars.chooseDrive), a

cd_Loop:    ; ������ ������
            call  chooseDrive_draw

            ; ���� ������� �������
            call  bios_getch

            ; ������� ������
            push  af
            call  chooseDrive_draw
            pop   af

            ; ����� ����� ��������� ������
            ld    c, 1
            cp    18h       ; ������
            jp z, cd_Arrows
            ld    c, 7
            cp    8         ; �����
            jp z, cd_Arrows
            ld    c, 4
            cp    1Ah       ; ����
            jp z, cd_Arrows
            cp    19h       ; �����
            jp z, cd_Arrows
            cp    0Dh       ; Enter
            jp z, cd_Enter
            cp    1Bh       ; Esc
            jp z, cd_Esc

            ; ����� ����� ��������� a-h
            sub   41h
            cp    8
            jp nc, cd_Loop
            ld    c, a

            ; ��������������� ��� �������
            pop   af
            ld    (bios_vars.cursorCfg),a
            ld    a, c
            ret

; ---------------------------------------------------------------------------

cd_Esc:     ; ��������������� ��� �������
            pop   af
            ld    (bios_vars.cursorCfg),a

            ; ������ �� �������
            ld    a, 0FFh
            ret

; ---------------------------------------------------------------------------

cd_Enter:   ; ��������������� ��� �������
            pop   af
            ld    (bios_vars.cursorCfg),a
            ; �������
            ld    a, (vars.chooseDrive)
            ret

; ---------------------------------------------------------------------------

cd_Arrows:  ld    a, (vars.chooseDrive)
            add   a, c
            and   7
            ld    (vars.chooseDrive),a
            jp    cd_Loop

; ---------------------------------------------------------------------------

chooseDrive_draw:
            ; ��������� ����������
            ld    a, (vars.chooseDrive)

            ; ���������� �������
            ld    hl, 976Ch

            ; ������ ������
            cp    4
            jp c, chooseDrive_draw2
            sub  4
            ld   c, a
            ld   a, l
            add  a, 0Bh
            ld   l, a
            ld   a, c
chooseDrive_draw2:      
            ; ������
            ld    c, a
            add   a, a
            add   a, c
            add   a, h
            ld    h, a
            call  activePanelPos
            ld    bc, (1 << 8) + 9  ; ������ 1 ����, ������ 9 ��������
            jp    inverseRect       ; hl = �����
            
