;----------------------------------------------------------------------------
; MXOS NC.COM
; Рисование информационной панели
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

printInfoPanel:
            COLOR   COLOR_BORDER
            call    drawPanelTop

            ; Рисуем панель
            ld      hl, g_infoPanel
            call    draw

printInfoContents:
            COLOR   COLOR_INFOHEADER

            ; Печать "COMMANDER VERSION XXX"
            ld      hl, 1010h
            call    setCursorPosPanel
            ld      hl, aCommanderVer
            call    bios_printString

            ; Печать "(C) OMSK 1992"
            ;ld      hl, 1D1Ah
            ld      hl, 101Ah
            call    setCursorPosPanel
            ld      hl, aCopyright    
            call    bios_printString

            ; Положение курсора
            ld      hl, 0C30h
            call    setCursorPosPanel

            ; Общий объем ОЗУ
            ld      de, 65504   ; Специалист-MX/MX2

            ; Печать "XXXX BYTES MEMORY"
            ld      hl, aKBytesMemory
            call    pip_printNumberSpace

            ; Положение курсора
            ld      hl, 0C3Ch
            call    setCursorPosPanel

            ; Свободный объем ОЗУ
            call    bios_getMemTop
            inc     hl  ; сободно на 1 байт больше, чем memTop
            ex      hl, de

            ; Печать "XXXX BYTES FREE"
            ld      hl, aKBytesFree
            call    pip_printNumberSpace

            ; Положение курсора
            ld      hl, 0448h
            call    setCursorPosPanel

            ; Общий объем ДОЗУ в кб
            ld      a, (bios_upper.ramPageCount)
            ; hl = a * 256
            ld      h, a
            ld      l, 0
            ; hl = hl / 4
            call    ncMath.div_hl_2
            call    ncMath.div_hl_2
            ex      hl, de

            ; Печать "XXXX KB EXTENDED MEMORY"
            ld      hl, aKBytesExtMemory
            call    pip_printNumberSpace

            ; Получить размер диска
            ld      b, 3    ; режим
            call    bios_diskDriver             ; de = количество кластеров на диске, hl = адрес DISK_INFO
            ld      (vars.diskInfoPtr), hl   ; cохраним адрес структуры DISK_INFO

            ; Печатаем фактический драйвер и для неформатированного диска.
            push    de
            call    pip_printDriver
            pop     de
            ld      hl, (vars.diskInfoPtr)

            ; Проверяем, распознан ли диск
            ld      bc, DISK_INFO.isValid
            add     hl, bc                  ; hl = адрес поля isValid
            ld      a, (hl)                 ; a = isValid
            cp      DISK_VALID              ; если значение меньше, чем DISK_VALID (2)
            jp c,   pip_printDiskError      ; то переходим сюда

            ;---------------------------------------------------------

pip_printDiskInfo:
            ; Положение курсора
            ld      hl, 045Ch
            call    setCursorPosPanel

            ; Считаем объем в килобайтах
            ; de = de * 256 / 1024 = hl / 4
            ex      hl, de
            call    ncMath.div_hl_2
            call    ncMath.div_hl_2
            ex      hl, de
            push    de  ; в стеке - объем диска в кб

            ; Печать "XXXX KBYTES TOTAL ON DRIVE A:"
            ld      hl, aKBytesTotalOnDrv
            call    pip_printNumberSpace
            COLOR   COLOR_INFONUMBER
            call    printCurDrive
            COLOR   COLOR_INFOTEXT

            ; Положение курсора
            ld      hl, 0C98h
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
            ld      hl, 0468h
            call    setCursorPosPanel
            
            ; Подсчет свободного места на диске.
            ; Эта п/п может работать довольно долго,
            ; в зависимости от размера fat на диске
            ; Результат: de = количество свободных кластеров
            MXOS_CALL MXOS_DOS_GET_DRIVE_FREE_SPACE

            ; Считаем объем в килобайтах
            ; hl = hl * 256 / 1024 = hl / 4
            ex      hl, de
            call    ncMath.div_hl_2
            call    ncMath.div_hl_2
            ex      hl, de
            push    de  ; в стеке - объем свободного места в кб

            ; Печать "XXXX KBYTES FREE ON DRIVE A:"
            ld      hl, aKBytesFreeOnDrv
            call    pip_printNumberSpace
            COLOR   COLOR_INFONUMBER
            call    printCurDrive

            ; Положение курсора
            ld      hl, 0474h
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
            ld      hl, aFilesUse
            call    pip_printNumberSpace

            pop     de  ; из стека - объем свободного места в кб
            pop     hl  ; из стека - объем диска в кб
            call    bios_sub_hl_de
            ex      hl, de

            ; Печать занятого объёма без переноса суффикса диска.
            ld      b, 0    ; не дополнять пробелами
            ld      hl, aKBytesIn
            jp      pip_printNumberText

            ; ^^^ выход

            ;---------------------------------------------------------

pip_printDiskError:
            ; Панель без драйвера теперь недостижима: выбор дисков и
            ; начальная инициализация оставляют только установленные драйверы.

            ; Положение курсора
            ld      hl, 0C5Ch
            call    setCursorPosPanel

            ; Печать "DRIVE A: IS NOT FORMATTED"
            COLOR   COLOR_INFOTEXT
            ld      hl, aDrive
            call    bios_printString
            COLOR   COLOR_INFONUMBER
            call    printCurDrive
            COLOR   COLOR_INFOTEXT
            ld      hl, aNotFormatted
            jp      bios_printString

            ; ^^^ выход

; Общий вывод числа и подписи вместо семи одинаковых последовательностей.
; Вход DE=число, HL=подпись, B=дополнение; Space задаёт ведущие пробелы.
; Выход HL=конец подписи, DE сохранён; AF/флаги и B разрушаются.
pip_printNumberSpace:
            ld      b, 20h
pip_printNumberText:
            COLOR   COLOR_INFONUMBER
            call    ncPrintDecWord
            COLOR   COLOR_INFOTEXT
            jp      bios_printString

; B=0FFh: HL=две строки, B=технология. AF/BC/DE/HL разрушаются.
; Строки ограничены контрактом ABI (22/30 символов).
pip_printDriver:
            ld      hl, 0CB0h
            call    setCursorPosPanel
            COLOR   COLOR_INFOTEXT
            ld      hl, aDiskDriver
            call    bios_printString
            ld      hl, 0
            ld      b, 0FFh
            call    bios_diskDriver
            jp c,   pip_driverUnknown
            ld      a, h
            or      l
            jp nz,  pip_driverName
pip_driverUnknown:
            ld      b, 0
            ld      hl, aDriverUnknown
pip_driverName:
            COLOR   COLOR_INFONUMBER
            call    bios_printString
            inc     hl
            push    hl
            ld      hl, 0CBCh
            call    setCursorPosPanel
            COLOR   COLOR_INFOTEXT
            pop     hl
            call    bios_printString

            ; BIOS печати сохраняет BC: тип из того же запроса B=FFh ещё в B.
            ld      hl, 0CC8h
            call    setCursorPosPanel
            ld      hl, aDiskTechnology
            call    bios_printString
            ld      a, b
            ld      hl, aTypeROM
            cp      DISK_TECH_ROM
            jp z,   pip_driverType
            ld      hl, aTypeRAM
            cp      DISK_TECH_RAM
            jp z,   pip_driverType
            ld      hl, aTypeFlash
            cp      DISK_TECH_FLASH
            jp z,   pip_driverType
            ld      hl, aDriverUnknown
pip_driverType:
            COLOR   COLOR_INFONUMBER
            jp      bios_printString
