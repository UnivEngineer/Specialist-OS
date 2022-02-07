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

            ; Печать "COMMANDER VERSION XXX"
            ld      hl, 1110h
            call    setCursorPosPanel
            ld      hl, aCommanderVer
            call    bios_printString

            ; Печать "(C) OMSK 1992"
            ld      hl, 1D1Ah
            ;ld      hl, 0E1Ah
            call    setCursorPosPanel
            ld      hl, aCopyright    
            call    bios_printString

            ; Положение курсора
            ld      hl, 0B30h
            call    setCursorPosPanel

            ; Общий объем ОЗУ
            ld      de, 65504   ; Специалист-MX/MX2

            ; Печать "XXXX BYTES MEMORY"
            COLOR   COLOR_INFONUMBER
            ld      b, 20h
            call    bios_printDecWord
            COLOR   COLOR_INFOTEXT
            ld      hl, aKBytesMemory
            call    bios_printString

            ; Положение курсора
            ld      hl, 0B3Ch
            call    setCursorPosPanel

            ; Свободный объем ОЗУ
            call    bios_getMemTop
            inc     hl  ; сободно на 1 байт больше, чем memTop
            ex      hl, de

            ; Печать "XXXX BYTES FREE"
            COLOR   COLOR_INFONUMBER
            ld      b, 20h
            call    bios_printDecWord
            COLOR   COLOR_INFOTEXT
            ld      hl, aKBytesFree
            call    bios_printString

            ; Положение курсора
            ld      hl, 0548h
            call    setCursorPosPanel

            ; Общий объем ДОЗУ в кб
            ld      a, (bios_vars.ramPageCount)
            ld      h, a
            ld      l, 0        ; hl = a * 256 - число кластеров
            call    div_hl_2
            call    div_hl_2    ; hl = a * 64 - объем в кб
            ex     hl, de

            ; Печать "XXXX KB EXTENDED MEMORY"
            COLOR   COLOR_INFONUMBER
            ld      b, 20h
            call    bios_printDecWord
            COLOR   COLOR_INFOTEXT
            ld      hl, aKBytesExtMemory
            call    bios_printString

            ; Положение курсора
            ld      hl, 055Ch
            call    setCursorPosPanel

            ; Получить размер диска
            ld      b, 3
            call    bios_diskDriver ; de = кол-во кластеров

            ; Надо вычесть размер fat и каталога
            ;ld      hl, FAT_CLUSTERS + DIR_CLUSTERS
            ex      hl, de
            ;call    sub_hl_de   ; hl = объем диска в кластерах

            ; Считаем обем в килобайтах
            ; hl = hl * 256 / 1024 = hl / 4
            call    div_hl_2
            call    div_hl_2
            ex      hl, de
            push    de  ; в стеке - объем диска в кб

            ; Печать "XXXX KBYTES TOTAL ON DRIVE A:"
            COLOR   COLOR_INFONUMBER
            ld      b, 20h
            call    bios_printDecWord
            COLOR   COLOR_INFOTEXT
            ld      hl, aKBytesTotalOnDrv
            call    bios_printString
            COLOR   COLOR_INFONUMBER
            call    printCurDrive
            COLOR   COLOR_INFOTEXT

            ; Положение курсора
            ld      hl, 0568h
            call    setCursorPosPanel
            
            ; Подсчет свободного места на диске
            ; Считаем не занятые кластеры, начиная с FAT_CLUSTERS + DIR_CLUSTERS
            ld      de, (FAT_CLUSTERS + DIR_CLUSTERS) * FAT_ITEM_SIZE
            ld      hl, FAT_BUFFER  ; hl = адрес таблицы fat в памяти
            add     hl, de          ; hl = адрес первой незарезервированной ячейки в таблице fat
            ld      de, 0           ; счетчик свободных кластеров
            ld      bc, FAT_SIZE / FAT_ITEM_SIZE ; общее кол-во ячеек fat

getFreeSpaceLoop:

            ; Читаем номер кластера из fat
            ld      a, (hl)
            inc     hl
            or      (hl)    ; ZF = 0, если кластер пустой (0)
            inc     hl      ; эта инструкция не меняет состояние флагов процессора

            ; Если кластер пустой, увеличиваем счетчик свободных кластеров
            jp nz,  getFreeSpaceLoopNext
            inc     de

getFreeSpaceLoopNext:
            dec     bc  ; счетчик просмотренных кластеров
            ld      a, b
            or      c
            jp nz,  getFreeSpaceLoop

            ; de = количество свободных кластеров.
            ; Считаем обем в килобайтах
            ; hl = hl * 256 / 1024 = hl / 4
            ex      hl, de
            call    div_hl_2
            call    div_hl_2
            ex      hl, de
            push    de  ; в стеке - объем диска в кб

            ; Печать "XXXX KBYTES FREE ON DRIVE A:"
            COLOR   COLOR_INFONUMBER
            ld      b, 20h
            call    bios_printDecWord
            COLOR   COLOR_INFOTEXT
            ld      hl, aKBytesFreeOnDrv
            call    bios_printString
            COLOR   COLOR_INFONUMBER
            call    printCurDrive

            ; Положение курсора
            ld      hl, 0574h
            call    setCursorPosPanel

            ; de = кол-во файлов          
            ld      hl, panelA_filesCnt
            ld      a, (activePanel)
            and     a
            jp nz,  loc_D8E0
            inc     hl
loc_D8E0:   ld      e, (hl)
            ld      d, 0

            ; Печать "XX FILES USE"
            COLOR   COLOR_INFONUMBER
            ld      b, 20h
            call    bios_printDecWord
            COLOR   COLOR_INFOTEXT
            ld      hl, aFilesUse
            call    bios_printString

            pop     de  ; из стека - объем свободного места в кб
            pop     hl  ; из стека - объем диска в кб
            call    sub_hl_de
            ex      hl, de

            ; Печать "XXXX BYTES IN"
            COLOR   COLOR_INFONUMBER
            ld      b, 0    ; не дополнять пробелами
            call    bios_printDecWord
            COLOR   COLOR_INFOTEXT
            ld      hl, aKBytesIn
            call    bios_printString

            ; Положение курсора
            ld      hl, 2C80h
            call    setCursorPosPanel

            ; Печать "A:"
            ; Здесь будет печататься путь к текущей папке...
            COLOR   COLOR_INFONUMBER
            jp      printCurDrive

