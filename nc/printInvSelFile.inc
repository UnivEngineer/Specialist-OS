;----------------------------------------------------------------------------
; MXOS NC.COM
; Инверсный вывод имени выбранного файла
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

printInvSelFile:
            call  getSelectedFile
            ex    de, hl
printInvFile:     ld    hl, 506Eh
            ld    (bios_vars.cursorY), hl
            ex    de, hl
            ld    de, vars.input
            call  compactName
            ld    hl, vars.input
            jp    printStringInv

