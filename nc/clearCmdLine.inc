;----------------------------------------------------------------------------
; MXOS NC.COM
; ������� ��� ������
;
; 2013-12-18 ����������������� vinxru
;----------------------------------------------------------------------------

clearCmdLine:
            ld    hl, 7F3h
            ld    (bios_vars.cursorY), hl

            ld    h, 10
            ld    (vars.cmdLinePos), hl

            COLOR   COLOR_CMDLINE
            
            ld    c, 92 ; '\'
            call  bios_printChar

            ld    hl, vars.cmdLine
            ld    (vars.cmdLineEnd), hl
                               
            ld    b, 60

            ; ����������� �� printSpaces

