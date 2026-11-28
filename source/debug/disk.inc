; Выход, каталог и дисковые директивы
; Порядок включения задан главным .asm; см. README.md.

Dir_J:
        MXOS_CALL_REGS MXOS_BIOS_KEY_CHECK
        INC   A
        JP    Z,mon2_init
        JP    Dir_J

;----------------------------------------------------------------------------
; Запись слова 0000h в начало буфера для ввода строки INPUT_BUF

ClearInputBuf:
        LD    HL,INPUT_BUF
        LD    (HL),00H
        INC   HL
        LD    (HL),00H
        RET

;----------------------------------------------------------------------------
; MXOS
;
; Директива ? — вывести каталог текущего диска и 16-битные диапазоны загрузки.
;
; вход:
;   Регистровых параметров нет. DOS использует текущий диск и каталог.
;
; выход:
;   Список напечатан до маркера FFh или остановлен клавишей СТР.
;   Буфер v_fileInfo заполнен через DOS. AF, BC, DE, HL и флаги разрушены.

Dir_DirDisk:
        LD    BC, 0                 ; начинаем с 0 файла
        LD    HL, v_fileInfo.name   ; адрес буфера
        LD    DE, DIR_BUFFER_SIZE   ; размер буфера в штуках
        MXOS_CALL_REGS MXOS_DOS_FILE_LIST
DirDiskLoop:
        LD    A,(HL)            ; первый символ имени файла
        INC   A
        RET   Z                 ; если FF (конец каталога) - выходим
        MXOS_CALL_REGS MXOS_BIOS_KEY_CHECK     ; Проверяем, не нажата ли клавиша
        CP    1FH               ; СТР
        CALL  Z,mon2_getchOld      ; если нажата СТР, ждем нажатия СТР еще раз
        CP    1FH               ; СТР
        RET   Z                 ; выход, если СТР нажата еще раз
        CALL  printFileNameReal ; 8 символов имени, точка и 3 расширения одним пакетом
        CALL  printSpace
        INC   HL                ; FILE_INFO.size идёт за атрибутом
        LD    E,(HL)            ; DE = младшее слово размера
        INC   HL
        LD    D,(HL)
        PUSH  HL
        LD    HL,0              ; начальный адрес для вывода каталога
        CALL  printHexWordHL
        ADD   HL,DE
        CALL  printHexWordHL    ; конечный адрес в 16-битном представлении
        POP   HL                ; HL = указатель дескриптора
        LD    A,L
        AND   0F0H
        LD    L,A               ; Обнуляем младшие 4 бита HL - переход на начало текущего дескриптора файла
        LD    DE,0010H
        ADD   HL,DE             ; HL += 16 - переход на следующий дескриптор файла
        CALL  printNewLine
        JP    DirDiskLoop

;----------------------------------------------------------------------------
;----------------------------------------------------------------------------
; Директива B - сохранение области памяити в файл

Dir_B:  ; адрес начала - в дескриптор
        LD    (v_dumpAddress), HL

        ; DE = -HL
        EX    DE,HL
        LD    A,D
        CPL
        LD    D,A
        LD    A,E
        CPL
        LD    E,A
        INC   DE

        ; HL = адрес_конца - адрес_начала = размер - 1
        ADD   HL,DE

        ; Точный размер N, конец директивы включительный.
        INC HL
        LD    (v_fileDescr.size),HL

        ; запрос именя файла
        CALL  EnterFileName
        RET   C

        ; DE — буфер, адрес не записывается в FAT.
        PUSH HL
        LD HL,(v_dumpAddress)
        EX DE,HL
        POP HL
        CALL  fileWriteDescriptor

        ; в случае ошибки печатаем надпись "МАЛ ДИСК"
        LD    HL,txtSmallDisk
        CALL  C,printCreateError
        RET

        ; или "МАЛ DIR"
printCreateError:   OR    A
        JP    Z,printDirectoryFull
        JP    mon2_printStringOld

printDirectoryFull:   LD    HL, txtSmallDir
        JP    mon2_printStringOld

;----------------------------------------------------------------------------
; Русский текст в кодировке КОИ-7

txtSmallDisk:   DB 0AH,0DH,"SMALL DISK!",00H
txtSmallDir:    DB 0AH,0DH,"SMALL DIR!",00H
txtNoFile:      DB 0AH,0DH,"NO FILE ",00H

;----------------------------------------------------------------------------
; Директива A - изменение адреса загрузки файла

Dir_A:  LD HL,txtHeaderAddress
        JP mon2_printStringOld
txtHeaderAddress:   DB 0AH,"ADDRESS IS IN FILE HEADER",0
v_dumpAddress:      DW 0

;----------------------------------------------------------------------------
; Директива V - загрузка файла в память по указанному адресу

Dir_V:  ; запоминаем первый параметр директивы в стеке
        PUSH  HL

        ; запрос имени файла
        CALL  EnterFileName
        RET   C

        ; вытаскиваем первый параметр директивы из стеке
        POP   DE
        PUSH  DE

        ; загружаем файл по адресу, указанному в директиве
        CALL  fileReadDescriptor

        ; если файл не найден - вывод сообщения "НЕТ ФАЙЛА" и выход
        PUSH  AF
        LD    HL,txtNoFile
        CALL  C,mon2_printStringOld
        POP   AF
        POP   DE
        RET   C

        ; получение информации о файле
        PUSH  DE
        LD    HL,v_fileDescr.name
        MXOS_CALL_REGS MXOS_DOS_FILE_LOAD_INFO ; размер файла
        LD    HL,(v_fileDescr.size)
        EX    DE,HL

        ; печать начального адреса
        POP   HL
        CALL  printHexWordHL

        ; печать конечного адреса
        ADD   HL,DE
        CALL  printHexWordHL
        RET

;----------------------------------------------------------------------------
; Директива U - загрузка файла в память

Dir_U:
        CALL EnterFileName
        RET   C
        CALL fileReadImage
        RET C
        LD H,B
        LD L,C
        PUSH HL
        EX DE,HL
        POP DE
        PUSH HL
        CALL printHexWordHL
        POP HL
        ADD HL,DE
        DEC HL
        JP printHexWordHL

;----------------------------------------------------------------------------
; Директива Y - установка атрибутов файла

Dir_Y:  PUSH  HL
        CALL  EnterFileName         ; ввод имени файла
        RET   C
        LD    C,01H                 ; режим для фаункции bios_fileGetSetAttr - установка байта атрибутов файла
        EX    (SP),HL
        LD    B,L
        POP   HL
        MXOS_CALL_REGS MXOS_DOS_FILE_GET_SET_ATTR ; установка байта атрибутов файла
        LD    HL,txtNoFile
        CALL  C,mon2_printStringOld    ; если файл не найден - вывести сообщение
        RET

;----------------------------------------------------------------------------
; Директива Q - печать атрибутов файла

Dir_Q:
        CALL  EnterFileName         ; ввод имени файла
        RET   C
        LD    C,02H                 ; режим для фаункции bios_fileGetSetAttr - чтение байта атрибутов файла
        MXOS_CALL_REGS MXOS_DOS_FILE_GET_SET_ATTR ; чтение байта атрибутов файла
        PUSH  AF                    ; результат и флаг ошибки (cf) - в стек
        LD    HL,txtNoFile
        CALL  C,mon2_printStringOld    ; если файл не найден - вывести сообщение
        CALL  printSpace
        POP   AF                    ; вытаскиваем байт атрибутов
        CALL  NC,mx_printHexByte  ; печатаем байт атрибутов, если не было ошибки
        RET

;----------------------------------------------------------------------------
; Запрос имени файла
; выход:
;   HL = адрес буфера с подготовленным именем файла

EnterFileName:
        LD    HL,txtFileQuestMark   ; печать приглашеиня ввести имя файла
        CALL  mon2_printStringOld
        LD    HL,INPUT_BUF          ; буфер для ввода строки
        LD    DE,INPUT_BUF_END
        MXOS_CALL_REGS MXOS_BIOS_INPUT
        LD    DE,v_fileDescr.name   ; буфер для подготовленного имени файла
        MXOS_CALL_REGS MXOS_DOS_FILE_NAME_PREPARE ; подготавливаем имя файла
        EX    DE,HL                 ; HL = подготовленное имя файла
        RET

