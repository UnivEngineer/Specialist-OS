; Приглашение, ввод и диспетчер директив
; Порядок включения задан главным .asm; см. README.md.

        JP    debugCheckMemory
debugMemoryReady:
        ; Начинаем с очистки экрана
        XOR   A
        call FONT_ENGINE_ENCODING      ; Имена и текстовые дампы — KOI-8.
        LD    C,1FH ; код символа очистки экрана
        CALL  mon2_printCharOld

        ; Рестарт без очистки экрана
RestartNoCls:
        LD    SP,STACK_ADDR ; Системный стек снова вне переменных DEBUG.
        LD    HL,debug_restart           ; установить обработчик ошибки магнитофона
        LD    (mon2_vars.tapeError),HL  ; на рестарт монитора
        LD    HL,7EFFH    ; 32511
        LD    (vars1.LegacyMemoryTop),HL
        CALL  MonitorMain
        JP    RestartNoCls

;----------------------------------------------------------------------------
; Главная подпрограмма - ввод директивы, анализ, выполнение

MonitorMain:
        ; перевод строки
        LD    HL,txtNewLine
        CALL  mon2_printStringOld

        ; узнать текущий диск
        LD    E,02H
        MXOS_CALL_REGS MXOS_DOS_FILE_GET_SET_DRIVE ; печать промпта A:\>
        ADD   A,'A'
        LD    C,A
        CALL  mon2_printCharOld
        LD    C,':'
        CALL  mon2_printCharOld
        LD    C,'\'
        CALL  mon2_printCharOld
        LD    C,'>'
        CALL  mon2_printCharOld

        ; очистка буфера
        CALL  ClearInputBuf

        ; ввод строки
        CALL  inputCommandLine

        PUSH  HL
        PUSH  DE
        PUSH  BC

        ; узнать текущий диск
        LD    E,02H
        MXOS_CALL_REGS MXOS_DOS_FILE_GET_SET_DRIVE
        PUSH  AF

        ; преобразовать имя файла в буфере
        LD    HL,INPUT_BUF
        LD    DE,v_fileInfo.name
        MXOS_CALL_REGS MXOS_DOS_FILE_NAME_PREPARE ; узнать текущий диск
        LD    E,02H
        MXOS_CALL_REGS MXOS_DOS_FILE_GET_SET_DRIVE
        LD    E,A
        POP   AF
        CP    E
        POP   BC
        POP   DE
        POP   HL
        RET   NZ

        ; перевод строки
        CALL  printNewLine

        ; анализ строки
        CALL  Tokenizer

        ; анализ дуквы директивы
        CP    44H           ; Директива D - дамп блока памяти в HEX виде
        JP    Z,Dir_D
        CP    4DH           ; Директива M - редактирование блока памяти
        JP    Z,Dir_M
        CP    4CH           ; Директива L - дамп блока памяти в текстовом виде
        JP    Z,Dir_L
        CP    4BH           ; Директива K - подсчет контрольной суммы блока памяти
        JP    Z,Dir_K
        CP    54H           ; Директива T - копирование блока памяти на новый адрес
        JP    Z,Dir_T
        CP    58H           ; Директива X - печать содержимого регистров процессора
        JP    Z,Dir_X
        CP    57H           ; Директива W - запись блока памяти на ленту без имени
        JP    Z,debug_tapeSave
        CP    52H           ; Директива R - чтение файла с ленты
        JP    Z,Dir_R
        CP    43H           ; Директива C - сравнение двух блоков памяти
        JP    Z,Dir_C
        CP    48H           ; Директива H - печать суммы и разности двух HEX слов
        JP    Z,Dir_H
        CP    4EH           ; Директива N - вывести байты, отличающиеся от третьего параметра
        JP    Z,Dir_N
        CP    47H           ; Директива G - запуск программы по адресу
        JP    Z,Dir_G
        CP    46H           ; Директива F - заполнить блок памяти байтом
        JP    Z,Dir_F
        CP    53H           ; Директива S - поиск последовательности из 1-4 байтов
        JP    Z,Dir_S
        CP    4AH           ; Директива J - выход из Mонитора
        JP    Z,Dir_J
        CP    3FH           ; Директива ? - листинг директории
        JP    Z,Dir_DirDisk
        CP    42H           ; Директива B - сохранить область памяти в файл
        JP    Z,Dir_B
        CP    41H           ; Директива A - изменение адреса загрузки файла
        JP    Z,Dir_A
        CP    56H           ; Директива V - загрузка файла в память по указанному адресу
        JP    Z,Dir_V
        CP    55H           ; Директива U - загрузка файла в память
        JP    Z,Dir_U 
        CP    59H           ; Директива Y - установка атрибутов файла
        JP    Z,Dir_Y
        CP    51H           ; Директива Q - печать атрибутов файла
        JP    Z,Dir_Q

        ; директива не распознана - печать знака вопроса и перезапуск
        JP    TypeErrorRestart

;----------------------------------------------------------------------------
; Директива J - выход из Mонитора

