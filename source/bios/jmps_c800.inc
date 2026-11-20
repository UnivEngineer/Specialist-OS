;+---------------------------------------------------------------------------
; MXOS
; Стандартные точки входа C800
;
; 2013-12-12 Дизассемблировано vinxru
;---------------------------------------------------------------------------

j_reboot3:      jp  reboot3             ; C800: Восстановить звук и запустить NC.COM
j_getch:        jp  getch               ; C803: Ожидание ввода с клавиатуры
j_tapeRead:     jp  t_tapeRead          ; C806: Чтение байта с магнитофона
j_printChar:    jp  FONT_ENGINE_CHAR           ; C809: Вывод символа на экран
j_tapeWrite:    jp  t_tapeWrite         ; C80C: Запись байта на магнитофон
                jp  input               ; C80F: Ввод строки с клавиатуры
                jp  j_keyScan           ; C812: Получить код нажатой клавиши
                jp  printHexByte        ; C815: Вывести 16-ричное число (байт)
j_printString:  jp  FONT_ENGINE_Z        ; C818: Вывести строку на экран
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
