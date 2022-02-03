;+---------------------------------------------------------------------------
; MXOS
; Стандартные точки входа C800
;
; 2013-12-12 Дизассемблировано vinxru
;---------------------------------------------------------------------------

j_reboot3:      jp  reboot3         ; C800: Запустить NC.COM
j_getch:        jp  getch           ; C803: Ожидание ввода с клавиатуры
j_tapeRead:     jp  t_tapeRead      ; C806: Чтение байта с магнитофона
j_printChar:    jp  printChar       ; C809: Вывод символа на экран
j_tapeWrite:    jp  t_tapeWrite      ; C80C: Запись байта на магнитофон
                jp  input           ; C80F: Ввод строки с клавиатуры
                jp  j_keyScan       ; C812: Получить код нажатой клавиши
                jp  printHexByte    ; C815: Вывести 16-ричное число (байт)
j_printString:  jp  printString1    ; C818: Вывести строку на экран
                jp  j_keyScan       ; C81B: Получить код нажатой клавиши
                jp  getCursorPos    ; C81E: Получить координаты курсора в hl (координаты в пикселях)
                jp  setCursorPos    ; C821: Установить координаты курсора из hl (координаты в пикселях)
                jp  t_tapeLoad      ; C824: Загрузить программу с магнитофона
                jp  t_tapeSave      ; C827: Сохранить программу на магнитофон
                jp  calcCS          ; C82A: Расчет контрольной суммы
                jp  printHexWord    ; C82D: Вывести 16-ричное число (слово)
                jp  getMemTop       ; C830: Получить объем доступной памяти
                jp  setMemTop       ; C833: Установить объем доступной памяти
                jp  printer         ; C836: Напечатать байт на принтере
    IF RAMFOS_COMPATIBILITY
                jp  strToHex        ; C839: [NEW] Преобразвоние строки в HEX формате в число
    ELSE
                jp  j_reboot3       ; C839: Запустить NC.COM
    ENDIF
                jp  reboot3         ; C83C: Запустить NC.COM
                jp  fileList        ; C83F: Получить список файлов
                jp  fileGetSetDrive ; C842: Получить/установить активное устройство
                jp  fileCreate      ; C845: Создать файл
                jp  fileLoad        ; C848: Загрузить файл по адресу из заголовка этого файла
                jp  fileDelete      ; C84B: Удалить файл
                jp  fileRename      ; C84E: Переименовать файл
                jp  fileLoadInfo    ; C851: Загрузить информацию о файле
                jp  fileGetSetAddr  ; C854: Получить/установить адрес загрузки файла
                jp  fileGetSetAttr  ; C857: Получить/установить атрибуты файла
                jp  fileNamePrepare ; C85A: Преобразовать имя файла во внутренний формат
                jp  fileExec        ; C85D: Запустить файл
                jp  installDriver   ; C860: Установить драйвер накопителя
j_diskDriver:   jp  diskDriver      ; C863: Драйвер выбранного диска
                jp  fileLoad2       ; C866: Загрузить файл по адресу de
                jp  printChar_real  ; C869: [NEW] Вывод символа на экран (только реальные символы)
                jp  printDecWord    ; C86C: [NEW] Вывод числа экран в десятичной форме
                jp  setRAMDDriver   ; C86F: [NEW] Установить драйвер доступа к ДОЗУ

