;+---------------------------------------------------------------------------
; MXOS
; ������������ (�����������)
;
; 2013-12-12 ����������������� vinxru
;----------------------------------------------------------------------------

reboot1:    ; ��� ���������� �����
            ld    a, 36h

            ; �������� ���
            ld    (IO_PAGE_RAM), a

            ; ��������� ����        
            ld    (IO_TIMER_MODE), a

            ; ������������� ����������
            ld    hl, initVars      ; ������
            ld    de, initVarsEnd   ; �����
            ld    bc, bios_vars     ; ����
            call  memcpy_bc_hl

            ; ������������� ���� fat
            call  resetSectorCache

            ; ��������� �����
    IF ENABLE_COLOR
            ld    a, COLOR_BIOS
            ld    (IO_COLOR), a
    ELSE
            ld    a, 0Fh  
            ld    (IO_KEYB_MODE), a 
    ENDIF

            ; ������� �����
            call    clearScreen

    IF LOAD_FONT
onceInitFont:
            jp    initFont
    ELSE
            jp    reboot2
    ENDIF

