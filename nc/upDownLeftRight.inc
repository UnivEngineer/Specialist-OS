;----------------------------------------------------------------------------
; MXOS NC.COM
; Клавиши перемещения курсора
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

Down:       ; Если curFile+1 < filesCnt, то curFile = curFile+1
            ld    hl, panelA_filesCnt
            call  readBytePanel
            ld    c, a
            ld    hl, panelA_curFile
            call  readBytePanel
            inc   a
            cp    c
            jp z, mainDriveChanged
            call  writeBytePanel
            jp    mainDriveChanged

; ---------------------------------------------------------------------------

Up:         ; Если curFile > 0, то curFile = curFile-1
            ld    hl, panelA_curFile
            call  readBytePanel
            dec   a
            jp M, mainDriveChanged
            call  writeBytePanel
            jp    mainDriveChanged

; ---------------------------------------------------------------------------

Left:       ; Если curFile > 18, то curFile = curFile-18, иначе curFile = 0
            ld    hl, panelA_curFile
            call  readBytePanel
            sub   18
            jp p, Left_1
            xor   a           
Left_1:           call  writeBytePanel
            jp    mainDriveChanged

; ---------------------------------------------------------------------------

Right:            ld    hl, panelA_filesCnt
            call  readBytePanel
            ld    c, a
            dec   c
            jp z, mainDriveChanged
            ld    hl, panelA_curFile
            call  readBytePanel
            add   a,18
            cp    c
            jp c, Right_1
            ld    a, c
Right_1:    call  writeBytePanel
            jp    mainDriveChanged

