;----------------------------------------------------------------------------
; MXOS NC.COM
; Общая функция для копирования и переноса файла
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

copyFileInt:
            ; Загрузка выбранного файла в память
            call  loadSelFileAt0
            push  de    ; Тут реальный адрес

            ; Подготавливаем имя    
            ex    de, hl
            ld    hl, vars.input
            call  bios_fileNamePrepare
            ex    de, hl

saveFileInt:
            ; hl = адрес дескриптора файла
            push  hl

            ; Устанавливаем адрес сохраняемого файла
            ld    de, FILE_DESCRIPTOR.loadAddress   ; смещение поля адреса относительно начала дескриптора
            add   hl, de
            ld    de, 0     ; записываем 0 в поле loadAddress дескриптора
            ld    (hl), e
            inc   hl
            ld    (hl), d

            ; Сохраняем файл
            pop   hl
            call  bios_fileCreate

            ; Изменяем адрес загрузки файла
            pop   de
            ld    c, 1
            call  bios_fileGetSetAddr

            ; Выходим, если нет ошибки
            ret nc      

            ; Рисуем окно
            call  drawWindow2

            ; Вывод "CAN'T CREATE FILE"
            COLOR   COLOR_DIALOG_ERR
            CENTER_LINE 18, 80h
            ld    hl, aCantCreateFile
            call  bios_printString

            ; Вывод имени
            COLOR COLOR_DIALOG
            ld    hl, 5090h
            ld    (bios_vars.cursorY), hl
            ld    hl, vars.input
            call  printStringInv

anyKey:           ; Ждем нажатия любой клавиши
            call  bios_keyScan
            inc   a
            jp z, anyKey

            ; Выход
            pop   hl
            jp    mainReload

