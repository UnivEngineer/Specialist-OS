;+---------------------------------------------------------------------------
; MXOS
; Стандартные точки входа C800
;
; 2013-12-12 Дизассемблировано vinxru
;---------------------------------------------------------------------------

j_reboot3:      jp  reboot3             ; C800: Восстановить звук и запустить NC.COM
j_getch:        jp  getch               ; C803: Ожидание ввода с клавиатуры
j_tapeRead:     jp  t_tapeRead          ; C806: Чтение байта с магнитофона
j_printChar:    jp  printChar           ; C809: Вывод символа на экран
j_tapeWrite:    jp  t_tapeWrite         ; C80C: Запись байта на магнитофон
                jp  input               ; C80F: Ввод строки с клавиатуры
                jp  j_keyScan           ; C812: Получить код нажатой клавиши
                jp  printHexByte        ; C815: Вывести 16-ричное число (байт)
j_printString:  jp  printString        ; C818: Вывести строку на экран
                jp  j_keyScan           ; C81B: Получить код нажатой клавиши
                jp  getCursorPos        ; C81E: Получить координаты курсора в hl (координаты в пикселях)
                jp  setCursorPos        ; C821: Установить координаты курсора из hl (координаты в пикселях)
                jp  t_tapeLoad          ; C824: Загрузить программу с магнитофона
                jp  t_tapeSave          ; C827: Сохранить программу на магнитофон
                jp  calcCS              ; C82A: Расчет контрольной суммы
                jp  printHexWord        ; C82D: Вывести 16-ричное число (слово)
                jp  setMemTop           ; C830: Установить границу памяти из HL (Monitor-2)
                jp  getMemTop           ; C833: Получить границу памяти в HL (Monitor-2)
                jp  printer             ; C836: Напечатать байт на принтере
    IF RAMFOS_COMPATIBILITY
                jp  strToHex            ; C839: [MXOS2] Преобразвоние строки в HEX формате в число
    ELSE
                jp  j_reboot3           ; C839: Запустить NC.COM
    ENDIF
                jp  reboot3             ; C83C: Запустить NC.COM
                jp  sysLegacy_fileList            ; C83F: Получить список файлов
                jp  sysLegacy_fileGetSetDrive     ; C842: Получить/установить активное устройство
                jp  sysLegacy_fileCreate          ; C845: Создать файл
                jp  sysLegacy_fileLoad            ; C848: Читать подготовленное тело по DE, без промежуточного образа
                jp  sysLegacy_fileDelete          ; C84B: Удалить файл
                jp  sysLegacy_fileRename          ; C84E: Переименовать файл
                jp  sysLegacy_fileLoadInfo        ; C851: Загрузить информацию о файле
                jp  sysLegacy_imageOpen           ; C854: открыть родной COM/EXE; HL=вход, DE=загрузка, BC=тело
                jp  sysLegacy_fileGetSetAttr      ; C857: Получить/установить атрибуты файла
                jp  sysLegacy_fileNamePrepare     ; C85A: Преобразовать имя файла во внутренний формат
                jp  sysLegacy_fileExec            ; C85D: Запустить файл
                jp  installDriver       ; C860: Установить драйвер накопителя
j_diskDriver:   jp  sysDiskDriver          ; C863: Драйвер выбранного диска
                jp  sysLegacy_fileLoad2           ; C866: Загрузить файл по адресу de
                jp  printCharReal       ; C869: [MXOS2] Вывод символа на экран (только реальные символы)
                jp  printDecWord        ; C86C: [MXOS2] Вывод числа экран в десятичной форме
                jp  setRAMDDriver       ; C86F: [MXOS2] Установить драйвер доступа к ДОЗУ
                jp  sysLegacy_getDriveFreeSpace   ; C872: [MXOS2] Получить оставшийся свободный объем диска
                jp  upperCase           ; C875: [MXOS2] Перевод кода символа КОИ-8 в верхний регистр
                jp  strcmp              ; C878: [MXOS2] Сравнение строк, чувствительное к регистру
                jp  stricmp             ; C87B: [MXOS2] Сравнение строк, нечувствительное к регистру
                jp  sub_hl_de           ; C87E: [MXOS2] Вычитание hl и de: hl = hl - de
                DS 9,0 ; C881..C889: арифметика перенесена в include/math.inc
                jp  sysLegacy_fileCreatePrepare   ; C88A: [MXOS2] подготовить файл
                DS 3,0 ; C88D..C88F: удалённый вход кластерного переноса
                jp  sysLegacy_installVolume       ; C890: зарегистрировать драйвер тома
                jp  sysLegacy_getVolume           ; C893: таблица и контекст драйвера тома
                jp  sysLegacy_mountVolume         ; C896: повторное монтирование тома
                jp  sysLegacy_flushVolume         ; C899: сохранить изменения тома
                jp  beepTimer           ; C89C: звук ВИ53 с ABI RAMFOS beep

                jp  diskWriteChecked    ; C89F: запись сектора с чтением и сравнением

                jp fast6.setEncoding ; C8A2: явный выбор KOI7/KOI8
                DS 3,0 ; C8A5..C8A7 свободны

                DS 12,0 ; C8A8..C8B3 свободны после переноса UI в программу

                jp  sysLegacy_fileAssociation     ; C8B4: поиск ассоциации в A:CLI.EXT

                jp  argNext             ; C8B7: следующее слово без изменения строки

                jp  sysLegacy_fileChangeDrive     ; C8BA: выбрать диск
                jp  sysLegacy_fileRename          ; C8BD: переименование на выбранном диске
                jp  sysLegacy_fileDelete          ; C8C0: удаление на выбранном диске
                jp  sysLegacy_fileCopy            ; C8C3: копирование с явным буфером
                jp  sysLegacy_fileGetDrives       ; C8C6: список доступных дисков

                jp  sysLegacy_dosGetError         ; C8C9: последняя причина ошибки DOS

                jp installBlockDriver   ; C8CC: физический блок 128/256/512
                jp sysLegacy_logicalSectorIO      ; C8CF: логический сектор активного тома
                jp sysLegacy_fileStream           ; C8D2: поток с курсором вызывающей программы
                jp sysLegacy_fileHandle           ; C8D5: открытый файл в памяти вызывающего
                DS 6,0 ; C8D8..C8DD: DOS резидентна в своём банке
                jp sysLegacy_directoryService     ; C8DE: текущий каталог, пути и список с каталогами
                jp sysLegacy_cliService          ; C8E1: общий CLI
                jp sysLegacy_setShell            ; C8E4: выбор оболочки
                jp setInputHook         ; C8E7: обработчик ожидания клавиатуры
                ASSERT $ == 0C8EAh
                jp blockIO             ; C8EA: физический запрос DOS -> BIOS
                ASSERT $ == 0C8EDh
                jp printStringRawZ     ; C8ED: ASCIIZ без управляющих кодов
                ASSERT $ == 0C8F0h
                jp printStringRawLim  ; C8F0: HL=буфер, BC=байты
                jp bankText.dispatch    ; C8F3: внутренний MEM TEXT
                ASSERT $ == 0C8F6h
