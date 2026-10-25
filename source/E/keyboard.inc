; Диспетчер клавиш и таблицы Esc-команд
; Порядок включения задан главным .asm; см. README.md.

keyInput2:
        LD    C,A               ; сохраняем в регистре c
        CP    7FH               ; Del - удалить символ над курсором
        JP    Z,keyHandler_Del
        CP    07H               ; F8 - отмена изменений в строке
        JP    Z,keyHandler_F8
        CP    01H               ; F2 - курсор на страницу вверх (PageUp)
        JP    Z,keyHandler_F2
        CP    02H               ; F3 - курсор на страницу вниз (PageDn)
        JP    Z,keyHandler_F3
        CP    20H               ; пробел
        JP    NC,keyHandler_Space
        OR    A                 ; F1 - Ins (перекл. режима Insert/Overwrite)
        JP    Z,keyHandler_F1

        ; Теперь проходм по таблице подпрограмм
        LD    HL, KeyHendlerTable
fkh_Loop:
        LD    A,(HL)        ; код клавиши
        OR    A             ; если 0 - достигнут конец таблицы
        JP    Z,Beep4       ; бип 4 раза и выход
        INC   HL
        LD    E,(HL)        ; de = адрес подпрограммы
        INC   HL
        LD    D,(HL)
        INC   HL
        CP    C             ; сравниваем a и запомненный в с код клавиши
        JP    NZ,fkh_Loop   ; если не равно, повтор цикла
        PUSH  DE            ; алрес п/п - в стек
        RET                 ; переход на адрес п/п

;----------------------------------------------------------------------

    STRUCT  KEY_HANDLER
key:        DB  0
address:    DW  0
    ENDS

        ; Таблица переходов на подпрограммы
        ; код символа, адрес
KeyHendlerTable:
        KEY_HANDLER  1Ah, keyHandler_Down   ; Вниз      - курсор вниз
        KEY_HANDLER  19h, keyHandler_Up     ; Вверх     - курсор вверх
        KEY_HANDLER  08h, keyHandler_Left   ; Влево     - курсор влево
        KEY_HANDLER  18h, keyHandler_Right  ; Вправо    - курсор вправо
        KEY_HANDLER  0Dh, keyHandler_Enter  ; Enter     - вставить строку ниже (не разбивает текущую строку!)
        KEY_HANDLER  09h, keyHandler_Tab    ; Tab       - следующая позиция 4, 9, 17, 25...; Shift — предыдущая
        KEY_HANDLER  1Fh, keyHandler_CTP    ; СТР       - выход без сохранения!
        KEY_HANDLER  02h, keyHandler_F3_2   ; недостижимая ветвь: код 02h перехвачен выше
        KEY_HANDLER  06h, keyHandler_F7     ; F7        - поиск со строки после верхней видимой строки
        KEY_HANDLER  0Ch, keyHandler_Home   ; Home      - курсор в начало строки
        KEY_HANDLER  0Ah, keyHandler_End    ; End (ПС)  - курсор в конец строки
        KEY_HANDLER  1Bh, keyHandler_Esc    ; Esc       - режим Esc-последовательности
        DB  00H

;----------------------------------------------------------------------

        ; Обработка нажатия Esc
keyHandler_Esc:
        CALL    commitLineEdit

        ; Ожидаем ввод второго символа Esc-последовательности
        CALL    bios_getch
        LD      C,A

        ; Проходм по второй таблице подпрограмм
        LD      HL,KeyHendlerTable2
        JP      fkh_Loop

;----------------------------------------------------------------------

        ; Таблица переходов на подпрограммы
        ; код символа, адрес
KeyHendlerTable2:
        KEY_HANDLER  0Ch, keyHandler_Esc_Home   ; Esc-Home - курсор в начало первой страницы
        KEY_HANDLER  0Ah, keyHandler_Esc_End    ; Esc-End  - курсор в начало последней страницы
        KEY_HANDLER  4Ch, keyHandler_Esc_L      ; Esc-L    - выделить текущую строку, второй раз Esc-L - выделить от первой выделенной первой до текущей
        KEY_HANDLER  44h, keyHandler_Esc_D      ; Esc-D    - удалить выделенные строки
        KEY_HANDLER  55h, keyHandler_Esc_U      ; Esc-U    - снять выделение строк
        KEY_HANDLER  4Fh, keyHandler_Esc_O      ; Esc-O    - сохранить файл на магнитофон
        KEY_HANDLER  49h, keyHandler_Esc_I      ; Esc-I    - загрузить файл с магнитофона
        KEY_HANDLER  56h, keyHandler_Esc_V      ; Esc-V    - сравнить данные с ленты с текстом в памяти
        KEY_HANDLER  47h, keyHandler_Esc_G      ; Esc-G    - загрузить файл с магнитофона и вставить в конец документа
        KEY_HANDLER  4Eh, keyHandler_Esc_N      ; Esc-N    - новый файл (очистить буфер)
        KEY_HANDLER  4Ah, keyHandler_Esc_J      ; Esc-J    - объединить текущую строку и следующую
        KEY_HANDLER  53h, keyHandler_Esc_S      ; Esc-S    - разбить строку в положении курсора
        KEY_HANDLER  43h, keyHandler_Esc_C      ; Esc-C    - вставить выделенные строки ниже текущей
        KEY_HANDLER  4Dh, keyHandler_Esc_M      ; Esc-M    - переместить выделенные строки ниже текущей
        DB  00H

;----------------------------------------------------------------------

