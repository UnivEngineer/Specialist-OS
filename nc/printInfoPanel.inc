;----------------------------------------------------------------------------
; MXOS NC.COM
; Рисование информационной панели
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

printInfoPanel:
            COLOR   COLOR_BORDER

            ; Рисуем панель
            ld      hl, g_infoPanel
            call    draw

            COLOR   COLOR_INFOHEADER

            ; COMMANDER VERSION xxx
            ld      hl, 1110h
            call    setCursorPosPanel
            ld      hl, aCommanderVer
            call    bios_printString

            ; (C) OMSK 1992
            ld      hl, 1D1Ah
            call    setCursorPosPanel
            ld      hl, aCOmsk1992    
            call    bios_printString

            ; Положение курсора
            ld      hl, 1630h
            call    setCursorPosPanel

            ; Получить размер диска
            ld      e, 3        
            call    bios_diskDriver
    IF FIX_FREE_SPACE_BUG
            sbc     4
    ENDIF
            ld      d, a
            ld      e, 0
            push    de

            ; ##### TOTAL BYTES
            COLOR   COLOR_INFONUMBER
            call    bios_printDecWord
            COLOR   COLOR_INFOTEXT
            ld      hl, aTotalBytes   
            call    bios_printString

            ; ON DRIVE A:
            ld      hl, 1F3Ch
            call    setCursorPosPanel
            ld      hl, aOnDrive      
            call    bios_printString       
            COLOR   COLOR_INFONUMBER
            call    printCurDrive
            COLOR   COLOR_INFOTEXT

            ; Положение курсора
            ld      hl, 548h
            call    setCursorPosPanel
            
            ; Подсчет свободного места на диске

            ; Читаем FAT
            ld      hl, 0FE00h
            ld      de, 2
            call    bios_diskDriver

            ; Считаем не занятые кластеры с 4-ого
            ld      e, l
            ld      l, 4
    IF FIX_FREE_SPACE_BUG
            xor     a
            ld      d, a
    ELSE
            ld      d, l
    ENDIF
loc_D8B8:   ld      a, (hl)
            and     a
            jp nz,  loc_D8BE
            inc     d
loc_D8BE:   inc     l
            jp nz,  loc_D8B8

            push    de

            ; ##### BYTES FREE ON DRIVE #:
            COLOR   COLOR_INFONUMBER
            call    bios_printDecWord
            COLOR   COLOR_INFOTEXT
            ld      hl, aBytesFreeOnDrv
            call    bios_printString
            call    printCurDrive

            ; Положение курсора
            ld      hl, 0D54h
            call    setCursorPosPanel

            ; de = кол-во файлов          
            ld      hl, panelA_filesCnt
            ld      a, (activePanel)
            and     a
            jp nz,  loc_D8E0
            inc     hl
loc_D8E0:   ld      e, (hl)
            ld      d, 0

            ; ##### FILES USE #####
            COLOR   COLOR_INFONUMBER
            call    bios_printDecWord
            COLOR   COLOR_INFOTEXT
            ld      hl, aFilesUse
            call    bios_printString
            pop     hl
            pop     de
            ld      a, d
            sub     h
            ld      d, a
            ld      e, 0
            COLOR   COLOR_INFONUMBER
            call    bios_printDecWord
            COLOR   COLOR_INFOTEXT

            ; BYTES IN A:
            ld      hl, 1F60h
            call    setCursorPosPanel
            ld      hl, aBytesIn
            call    bios_printString       

            COLOR   COLOR_INFONUMBER
            jp      printCurDrive

