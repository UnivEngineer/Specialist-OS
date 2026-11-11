; Выход, каталог и дисковые директивы
; Порядок включения задан главным .asm; см. README.md.

Dir_J:  CALL  bios_keyCheck
        INC   A
        JP    Z,bios_reboot
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
; Директива ? - вывод каталога диска

Dir_DirDisk:
        LD    BC, 0                 ; начинаем с 0 файла
        LD    HL, v_fileInfo.name   ; адрес буфера
        LD    DE, DIR_BUFFER_SIZE   ; размер буфера в штуках
        CALL  bios_fileList
DirDiskLoop:
        LD    A,(HL)            ; первый символ имени файла
        INC   A
        RET   Z                 ; если FF (конец каталога) - выходим
        CALL  bios_keyCheck     ; Проверяем, не нажата ли клавиша
        CP    1FH               ; СТР
        CALL  Z,bios_getch      ; если нажата СТР, ждем нажатия СТР еще раз
        CP    1FH               ; СТР
        RET   Z                 ; выход, если СТР нажата еще раз
        CALL  printFileNameReal ; 8 символов имени, точка и 3 расширения одним пакетом
        CALL  printSpace
        INC   HL                ; переходим через байт атрибута файла
        LD    E,(HL)            ; DE = адрес загрузки файла
        INC   HL
        LD    D,(HL)
        PUSH  DE                ; адрес загрузки файла в стеке
        INC   HL
        LD    E,(HL)            ; DE = размер файла
        INC   HL
        LD    D,(HL)
        EX    (SP),HL           ; текущий указатель дескриптора в стек, а HL = адрес загрузки файла
        CALL  printHexWordHL    ; печатаем начальный адрес загрузки файла
        ADD   HL,DE
        CALL  printHexWordHL    ; печатаем конечынй адрес загрузки файла
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
        CALL  bios_fileCreate

        ; в случае ошибки печатаем надпись "МАЛ ДИСК"
        LD    HL,txtSmallDisk
        CALL  C,printCreateError
        RET

        ; или "МАЛ DIR"
printCreateError:   OR    A
        JP    Z,printDirectoryFull
        JP    bios_printString

printDirectoryFull:   LD    HL, txtSmallDir
        JP    bios_printString

;----------------------------------------------------------------------------
; Русский текст в кодировке КОИ-7

txtSmallDisk:   DB 0AH,0DH,"SMALL DISK!",00H
txtSmallDir:    DB 0AH,0DH,"SMALL DIR!",00H
txtNoFile:      DB 0AH,0DH,"NO FILE ",00H

;----------------------------------------------------------------------------
; Директива A - изменение адреса загрузки файла

Dir_A:  LD HL,txtHeaderAddress
        JP bios_printString
txtHeaderAddress: DB 0AH,"ADDRESS IS IN FILE HEADER",0
v_dumpAddress: DW 0

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
        CALL  bios_fileLoad2

        ; если файл не найден - вывод сообщения "НЕТ ФАЙЛА" и выход
        PUSH  AF
        LD    HL,txtNoFile
        CALL  C,bios_printString
        POP   AF
        POP   DE
        RET   C

        ; получение информации о файле
        PUSH  DE
        LD    HL,v_fileDescr.name
        CALL  bios_fileLoadInfo

        ; размер файла
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
        CALL bios_imageOpen
        RET C
        PUSH DE
        PUSH BC
        CALL bios_fileLoad
        POP DE
        POP HL
        RET C
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
        CALL  bios_fileGetSetAttr   ; установка байта атрибутов файла
        LD    HL,txtNoFile
        CALL  C,bios_printString    ; если файл не найден - вывести сообщение
        RET

;----------------------------------------------------------------------------
; Директива Q - печать атрибутов файла

Dir_Q:
        CALL  EnterFileName         ; ввод имени файла
        RET   C
        LD    C,02H                 ; режим для фаункции bios_fileGetSetAttr - чтение байта атрибутов файла
        CALL  bios_fileGetSetAttr   ; чтение байта атрибутов файла
        PUSH  AF                    ; результат и флаг ошибки (cf) - в стек
        LD    HL,txtNoFile
        CALL  C,bios_printString    ; если файл не найден - вывести сообщение
        CALL  printSpace
        POP   AF                    ; вытаскиваем байт атрибутов
        CALL  NC,bios_printHexByte  ; печатаем байт атрибутов, если не было ошибки
        RET

;----------------------------------------------------------------------------
; Запрос имени файла
; выход:
;   HL = адрес буфера с подготовленным именем файла

EnterFileName:
        LD    HL,txtFileQuestMark   ; печать приглашеиня ввести имя файла
        CALL  bios_printString
        LD    HL,INPUT_BUF          ; буфер для ввода строки
        LD    DE,INPUT_BUF_END
        CALL  bios_input
        LD    DE,v_fileDescr.name   ; буфер для подготовленного имени файла
        CALL  bios_fileNamePrepare  ; подготавливаем имя файла
        EX    DE,HL                 ; HL = подготовленное имя файла
        RET

