;----------------------------------------------------------------------------
; MXOS NC.COM
; Очистка ком строки
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

clearCmdLine:
            ld    hl, 7F3h
            ld    (bios_vars.cursorY), hl

            ld    h, 10
            ld    (nc_vars.cmdLinePos), hl

            COLOR   COLOR_CMDLINE
            
            ld    c, 92 ; '\'
            call  bios_printChar

            ld    hl, nc_vars.cmdLine
            ld    (nc_vars.cmdLineEnd), hl
                               
            ld    b, 60

            ; Продолжение на printSpaces

