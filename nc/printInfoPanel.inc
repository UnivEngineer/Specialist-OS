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
            ;ld      hl, 1D1Ah
            ld      hl, 0E1Ah
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
            ; hl = a * 256
            ld      h, a
            ld      l, 0
            ; hl = hl / 4
            call    bios_div_hl_2
            call    bios_div_hl_2
            ex      hl, de

            ; Печать "XXXX KB EXTENDED MEMORY"
            COLOR   COLOR_INFONUMBER
            ld      b, 20h
            call    bios_printDecWord
            COLOR   COLOR_INFOTEXT
            ld      hl, aKBytesExtMemory
            call    bios_printString

            ; Получить размер диска
            ld      b, 3    ; режим
            call    bios_diskDriver             ; de = количество кластеров на диске, hl = адрес DISK_INFO
            ld      (vars.diskInfoPtr), hl   ; cохраним адрес структуры DISK_INFO

            ; Проверяем, распознан ли диск
            ld      bc, DISK_INFO.isValid
            add     hl, bc                  ; hl = адрес поля isValid
            ld      a, (hl)                 ; a = isValid
            cp      DISK_VALID              ; если значение меньше, чем DISK_VALID (2)
            jp c,   pip_printDiskError      ; то переходим сюда

            ;---------------------------------------------------------

pip_printDiskInfo:
            ; Положение курсора
            ld      hl, 055Ch
            call    setCursorPosPanel

            ; Считаем объем в килобайтах
            ; de = de * 256 / 1024 = hl / 4
            ex      hl, de
            call    bios_div_hl_2
            call    bios_div_hl_2
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
            ld      hl, 0B98h
            call    setCursorPosPanel

            ; Печать "Volume label: XXXXXXXXXXX"
            COLOR   COLOR_INFOTEXT
            ld      hl, aVolumeLabel
            call    bios_printString
            COLOR   COLOR_INFONUMBER
            ; hl = адрес поля v_diskInfo.volumeLabel
            ld      hl, (vars.diskInfoPtr)
            ld      de, DISK_INFO.volumeLabel
            add     hl, de
            call    bios_printString

            ; Положение курсора
            ld      hl, 0568h
            call    setCursorPosPanel
            
            ; Подсчет свободного места на диске.
            ; Эта п/п может работать довольно долго,
            ; в зависимости от размера fat на диске
            ; Результат: de = количество свободных кластеров
            call    bios_getDriveFreeSpace

            ; Считаем объем в килобайтах
            ; hl = hl * 256 / 1024 = hl / 4
            ex      hl, de
            call    bios_div_hl_2
            call    bios_div_hl_2
            ex      hl, de
            push    de  ; в стеке - объем свободного места в кб

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
            ld      hl, state.panelA_filesCnt
            ld      a, (state.activePanel)
            and     a
            jp nz,  pip_filesPresent
            inc     hl
pip_filesPresent:
            ld      e, (hl)
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
            call    bios_sub_hl_de
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

            ; ^^^ выход

            ;---------------------------------------------------------

pip_printDiskError:
            push    af  ; в a - код 0 (нет драйвера) или 1 (диск не отформатирован)

            ; Положение курсора
            ld      hl, 0B5Ch
            call    setCursorPosPanel

            ; Печать "DRIVE A:"
            COLOR   COLOR_INFOTEXT
            ld      hl, aDrive
            call    bios_printString
            COLOR   COLOR_INFONUMBER
            call    printCurDrive
            COLOR   COLOR_INFOTEXT

            ; Печать " HAS NO DRIVER" или " IS NOT FORMATTED"
            pop     af
            or      a
            jp z,   pip_printDiskNoDriver
            ld      hl, aNotFormatted
            jp      bios_printString

            ; ^^^ выход

pip_printDiskNoDriver:
            ld      hl, aHasNoDriver
            jp      bios_printString

            ; ^^^ выход
