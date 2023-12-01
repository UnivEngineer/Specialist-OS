;----------------------------------------------------------------------------
; MXOS - MON2.COM
;
; 2022-02-07 Дизассемблировано SpaceEngineer
;----------------------------------------------------------------------------

    INCLUDE "../include/mxos.inc"

; Используемые подпрограммы DOS.SYS:
; bios_keyScanOld      = 0C003h
; bios_drawCursorOld   = 0C006h
; bios_printCharOld    = 0C037h
; bios_beep_Old        = 0C170h
; bios_getchOld        = 0C337h
; bios_tapeReadOld     = 0C377h
; bios_tapeWriteOld    = 0C3D0h
; bios_cmp_hl_de       = 0C427h
; bios_printStringOld  = 0C438h
; bios_reboot          = 0C800h
; bios_getch           = 0C803h
; bios_printChar       = 0C809h
; bios_input           = 0C80Fh
; bios_keyCheck        = 0C812h
; bios_printHexByte    = 0C815h
; bios_printString     = 0C818h
; bios_keyScan         = 0C81Bh
; bios_getCursorPos    = 0C81Eh
; bios_tapeLoad        = 0C824h
; bios_tapeSave        = 0C827h
; bios_calcCS          = 0C82Ah
; bios_getMemTop       = 0C830h
; bios_setMemTop       = 0C833h
; bios_printer         = 0C836h
; bios_fileList        = 0C83Fh
; bios_fileGetSetDrive = 0C842h
; bios_fileCreate      = 0C845h
; bios_fileLoad        = 0C848h
; bios_fileLoadInfo    = 0C851h
; bios_fileGetSetAddr  = 0C854h
; bios_fileGetSetAttr  = 0C857h
; bios_fileNamePrepare = 0C85Ah
; bios_fileLoad2       = 0C866h

; Используемые переменные DOS.SYS:
; bios_vars.tapeError  = 8FE1h
; bios_vars.tapeAddr   = 8FE3h
; bios_vars.cursorCfg  = 8FE9h
; bios_vars.koi7       = 8FEAh
; bios_vars.cursorX    = 8FFCh

;----------------------------------------------------------------------------
; Собственные переменные
INPUT_BUF     = 8F60h
INPUT_BUF_END = 8F6Dh

    STRUCT MON2_VARS_1
Directive:      DS    1
FirstParam:     DS    2
SecondParam:    DS    2
ThirdParam:     DS    2
V_8F87:         DS    2
V_8F89:         DS    1
V_8F8A:         DS    1
V_8F8B:         DS    1
V_8F8C:         DS    2
V_8F8E:         DS    2
V_8F90:         DS    1
V_8F91:         DS    4
V_8F95:         DS    1
V_8F96:         DS    1
V_8F97:         DS    2
V_8F99:         DS    1
V_8F9A:         DS    3
V_8F9D:         DS    2
V_8F9F:         DS    2
V_8FA1:         DS    2
V_8FA3:         DS    2
V_8FA5:         DS    2
V_8FA7:         DS    4
V_8FAB:         DS    2
    ENDS

    STRUCT MON2_VARS_2
V_F0E0:         DS    5
V_F0E5:         DS    1
V_F0E6:         DS    1
V_F0E7:         DS    1
    ENDS

;----------------------------------------------------------------------------
; Начало программы
    ORG   0F100h

; Адреса блоков переменных
vars1       MON2_VARS_1     = 08F80H
vars2       MON2_VARS_2     = 0F0E0H
v_fileDescr FILE_DESCRIPTOR = 0F900H

; Буфер для листинга директории
; В новой карте памяти можно расположить его
; перед Монитором - там больше места
    IF  NEW_MEMORY_MAP
v_fileInfo  FILE_INFO = 0E800H
DIR_BUFFER_SIZE = ($ - v_fileInfo) / FILE_INFO_SIZE - 1
    ELSE
v_fileInfo  FILE_INFO = v_fileDescr + FILE_DESCRIPTOR_SIZE
DIR_BUFFER_SIZE = (0FB00h - v_fileInfo) / FILE_INFO_SIZE - 1
    ENDIF

;----------------------------------------------------------------------------
; Код

        ; Начинаем с очистки экрана
        LD    C,1FH ; rjl символа очистки экрана
        CALL  bios_printChar

        ; Рестарт без очистки экрана
RestartNoCls:
        LD    SP,0FFBFH
        LD    HL,mon2_restart           ; установить обработчик ошибки магнитофона
        LD    (bios_vars.tapeError),HL  ; на рестарт монитора
        LD    HL,7EFFH    ; 32511
        LD    (vars1.V_8FAB),HL
        CALL  MonitorMain
        JP    RestartNoCls

;----------------------------------------------------------------------------
; Главная подпрограмма - ввод директивы, анализ, выполнение

MonitorMain:
        ; перевод строки
        LD    HL,txtNewLine
        CALL  bios_printStringOld

        ; узнать текущий диск
        LD    E,02H
        CALL  bios_fileGetSetDrive

        ; печать промпта A:\>
        ADD   A,'A'
        LD    C,A
        CALL  bios_printChar
        LD    C,':'
        CALL  bios_printChar
        LD    C,'\'
        CALL  bios_printChar
        LD    C,'>'
        CALL  bios_printChar

        ; очистка буфера
        CALL  ClearInputBuf

        ; ввод строки
        CALL  SUB19

        PUSH  HL
        PUSH  DE
        PUSH  BC

        ; узнать текущий диск
        LD    E,02H
        CALL  bios_fileGetSetDrive
        PUSH  AF

        ; преобразовать имя файла в буфере
        LD    HL,INPUT_BUF
        LD    DE,v_fileInfo.name
        CALL  bios_fileNamePrepare

        ; узнать текущий диск
        LD    E,02H
        CALL  bios_fileGetSetDrive

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
        JP    Z,mon2_tapeSave
        CP    52H           ; Директива R - чтение файла с ленты
        JP    Z,Dir_R
        CP    43H           ; Директива C - сравнение двух блоков памяти
        JP    Z,Dir_C
        CP    48H           ; Директива H - печать суммы и разности двух HEX слов
        JP    Z,Dir_H
        CP    4EH           ; Директива N - ???
        JP    Z,Dir_N
        CP    47H           ; Директива G - запуск программы по адресу
        JP    Z,Dir_G
        CP    46H           ; Директива F - заполнить блок памяти байтом
        JP    Z,Dir_F
        CP    53H           ; Директива S - ???
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
        LD    B,08H             ; печатаем 8 символов имени файла
        CALL  printStringB
        LD    C,2EH             ; печатаем точку
        CALL  bios_printChar
        LD    B,03H             ; печатаем 3 символа расширения файла
        CALL  printStringB
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
; Печать строки по адресу HL длиной B

printStringB:
        LD    C,(HL)
        CALL  bios_printChar
        INC   HL
        DEC   B
        JP    NZ,printStringB
        RET

;----------------------------------------------------------------------------
; Директива B - сохранение области памяити в файл

Dir_B:  ; адрес начала - в дескриптор
        LD    (v_fileDescr.loadAddress), HL

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

        ; размер - в дескриптор
        LD    (v_fileDescr.size),HL

        ; запрос именя файла
        CALL  EnterFileName

        ; сохранение файла
        CALL  bios_fileCreate

        ; в случае ошибки печатаем надпись "МАЛ ДИСК"
        LD    HL,txtSmallDisk
        CALL  C,SUB4
        RET

        ; или "МАЛ DIR"
SUB4:   OR    A
        JP    Z,LBL6
        JP    bios_printString

LBL6:   LD    HL, txtSmallDir
        JP    bios_printString

;----------------------------------------------------------------------------
; Русский текст в кодировке КОИ-7

txtSmallDisk:   DB  0AH,0DH,"SMALL DISK!",00H
txtSmallDir:    DB  0AH,0DH,"SMALL DIR!",00H
txtNoFile:      DB  0AH,0DH,"NO FILE ",00H

;----------------------------------------------------------------------------
; Директива A - изменение адреса загрузки файла

Dir_A:  PUSH  HL
        CALL  EnterFileName
        POP   DE
        LD    C,01H
        CALL  bios_fileGetSetAddr
        LD    HL,txtNoFile
        CALL  C,bios_printString
        RET

;----------------------------------------------------------------------------
; Директива V - загрузка файла в память по указанному адресу

Dir_V:  ; запоминаем первый параметр директивы в стеке
        PUSH  HL

        ; запрос имени файла
        CALL  EnterFileName

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

Dir_U:  ; запрос имени файла
        CALL  EnterFileName
        CALL  bios_fileLoad

        ; если файл не найден - вывод сообщения "НЕТ ФАЙЛА" и выход
        PUSH  AF
        LD    HL,txtNoFile
        CALL  C,bios_printString
        CALL  printSpace
        POP   AF
        RET   C

        ; получение информации о файле
        LD    HL,v_fileDescr.name
        CALL  bios_fileLoadInfo

        ; печать адреса загрузки
        LD    HL,(v_fileDescr.loadAddress)
        PUSH  HL
        CALL  printHexWordHL
        POP   DE

        ; печать размера файла
        LD    HL,(v_fileDescr.size)
        ADD   HL,DE
        CALL  printHexWordHL
        RET

;----------------------------------------------------------------------------
; Директива Y - установка атрибутов файла

Dir_Y:  PUSH  HL
        CALL  EnterFileName         ; ввод имени файла
        LD    C,01H                 ; режим для фаункции bios_fileGetSetAttr - установка байта атрибутов файла
        EX    (SP),HL
        LD    A,L
        POP   HL
        CALL  bios_fileGetSetAttr   ; установка байта атрибутов файла
        LD    HL,txtNoFile
        CALL  C,bios_printString    ; если файл не найден - вывести сообщение
        RET

;----------------------------------------------------------------------------
; Директива Q - печать атрибутов файла

Dir_Q:
        CALL  EnterFileName         ; ввод имени файла
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

;----------------------------------------------------------------------------
; Директива R - чтеине файла с ленты

Dir_R:  ; загрузка файла
        CALL  mon2_tapeLoad
        PUSH  BC
        PUSH  DE
        PUSH  HL

        ; расчет контрольной суммы
        CALL  bios_calcCS

        ; печать начального адреса
        POP   HL
        CALL  printHexWordHL

        ; печать конечного адреса
        POP   HL
        CALL  printHexWordHL

        ; сравнение контрольной суммы из файла и рассчитанной
        POP   HL
        LD    D,B
        LD    E,C
        CALL  bios_cmp_hl_de

        ; если не совпадают - переход
        JP    NZ,LBL13

        ; иначе печать контрольной суммы и выход
        CALL  printHexWordHL
        RET

LBL13:  ; печать знака вопроса и выход
        CALL  printSpace
        LD    C, '?'
        CALL  bios_printChar
        RET

;----------------------------------------------------------------------------

SUB6:   PUSH  AF
        LD    A,(bios_vars.cursorCfg)
        PUSH  AF
        LD    A,11H
        JP    LBL63

        ; ???
        NOP
        NOP

;----------------------------------------------------------------------------
; Директива H - печать суммы и разности двух HEX слов

Dir_H:  PUSH  HL
        ADD   HL,DE             ; сумма
        CALL  printHexWordHL    ; печатаем ее
        CALL  printSpace        ; печатаем проблел

        ; DE = -DE
        LD    A,E
        CPL
        LD    E,A
        LD    A,D
        CPL
        LD    D,A
        INC   DE

        POP   HL
        ADD   HL,DE             ; разность
        CALL  printHexWordHL    ; печатаем ее
        RET

;----------------------------------------------------------------------------
; Директива X - печать содержимого регистров процессора

Dir_X:  LD    HL,vars1.V_8FA3+1 ; 36772
        LD    DE,txtRegisters             ; 63244
        LD    C,04H         ; 4
LBL16:  PUSH  BC
        CALL  SUB7
        POP   BC
        DEC   C
        JP    NZ,LBL16
        CALL  SUB8
        LD    HL,(vars1.V_8F9D)
        CALL  printHexWordHL
        CALL  SUB8
        LD    HL,(vars1.V_8F97)
        CALL  printHexWordHL
        CALL  SUB8
        LD    HL,(vars1.V_8FA5)
        CALL  printHexWordHL
        RET

;----------------------------------------------------------------------------
; Если нажата клавиша, записать 0FFh по адресу (HL)

setMIfKeyPressed:
        CALL  bios_keyScan
        INC   A
        RET   Z
        LD    (HL), 0FFh
        RET

;----------------------------------------------------------------------------

SUB7:   LD    B,(HL)
        DEC   HL
        LD    C,(HL)
        DEC   HL
        PUSH  BC
        CALL  SUB8
        LD    A,B
        CALL  mon2_printHexByte
        CALL  SUB8
        POP   BC
        LD    A,C
        CALL  mon2_printHexByte
        RET
SUB8:   EX    DE,HL         ; [5]
        PUSH  BC
        CALL  printSpace
        CALL  bios_printStringOld
        INC   HL
        POP   BC
        EX    DE,HL
        RET

;----------------------------------------------------------------------------
; Директива G - запуск программы по адресу (первый параметр)
; Второй параметр - точка перехвата?

Dir_G:  LD    A,E
        OR    A
        JP    NZ,LBL19  ; если в E не ноль
        LD    A,D
        OR    A
        JP    Z,LBL20   ; если в D не ноль

LBL19:  PUSH  HL
        EX    DE,HL
        LD    (vars1.V_8F97),HL
        LD    A,(HL)
        LD    (HL),0FFH
        LD    (vars1.V_8F99),A
        LD    HL,0038H
        LD    DE,vars1.V_8F9A
        LD    BC,REF4
        CALL  SUB11
        LD    (HL),0C3H
        CALL  SUB10
        CALL  SUB10
        LD    (HL),B
        DEC   HL
        LD    (HL),C
        POP   HL
LBL20:  CALL  SUB9
        JP    RestartNoCls
SUB9:   JP    (HL)
SUB10:  INC   HL
        INC   DE
SUB11:  LD    A,(HL)
        LD    (DE),A
        RET

REF4:   LD    (vars1.V_8F9D),HL
        EX    DE,HL
        LD    (vars1.V_8F9F),HL
        PUSH  BC
        POP   HL
        LD    (vars1.V_8FA1),HL
        PUSH  AF
        POP   HL
        LD    (vars1.V_8FA3),HL
        LD    HL,0000H
        ADD   HL,SP
        INC   HL
        INC   HL
        LD    (vars1.V_8FA5),HL
        LD    HL,(vars1.V_8F97)
        LD    A,(vars1.V_8F99)
        LD    (HL),A
        LD    HL,vars1.V_8F9A
        LD    DE,0038H
        CALL  SUB11
        CALL  SUB10
        CALL  SUB10
        JP    RestartNoCls

;----------------------------------------------------------------------------
; Директива S - ???

Dir_S:  PUSH  HL
        PUSH  BC
        LD    HL,INPUT_BUF
        LD    C,00H
LBL22:  LD    A,(HL)
        CP    2CH           ; символ ','
        JP    NZ,LBL23
        INC   C             ; пропускаем ','
LBL23:  INC   HL
        CP    0DH           ; пеервод строки
        JP    NZ,LBL22
        DEC   C
        LD    A,C
        LD    (vars1.V_8F89),A
        CP    01H
        POP   BC
        POP   HL
        JP    Z,LBL24
        PUSH  HL
        PUSH  DE
        PUSH  BC
        LD    HL,(vars1.V_8F87)
        LD    C,L
        LD    B,H
        CALL  SUB15
        LD    A,L
        LD    (vars1.V_8F8B),A
        LD    HL,vars1.V_8F8C
        LD    (HL),E
        INC   HL
        LD    (HL),C
        POP   BC
        POP   DE
        POP   HL
LBL24:  LD    A,C
        LD    (vars1.V_8F8A),A
LBL25:  PUSH  HL
        LD    HL,vars1.V_8F8A
        LD    (vars1.V_8F8E),HL
        POP   HL
        CALL  bios_cmp_hl_de
        RET   Z
        CALL  WaitClsKey
        LD    A,(vars1.V_8F89)
        LD    (vars1.V_8F90),A
        LD    B,A
LBL26:  LD    C,(HL)
        PUSH  HL
        LD    HL,(vars1.V_8F8E)
        LD    A,(HL)
        CP    C
        JP    NZ,LBL30
        INC   HL
        LD    (vars1.V_8F8E),HL
        POP   HL
        INC   HL
        DEC   B
        JP    NZ,LBL26
        PUSH  HL
        PUSH  DE
        PUSH  BC
        PUSH  AF
        LD    A,(vars1.V_8F89)
LBL27:  DEC   HL
        DEC   A
        JP    NZ,LBL27
        CALL  printHexWordHL
        DEC   HL
        DEC   HL
        LD    A,(vars1.V_8F89)
        ADD   A,04H         ; 4
        LD    B,A
LBL28:  CALL  SUB12
        INC   HL
        DEC   B
        JP    NZ,LBL28
        LD    HL,txtHomeRight11Dot
        CALL  mon2_printString
        LD    A,(vars1.V_8F89)
        LD    B,A
        ADD   A,B
        ADD   A,B
        DEC   A
        LD    E,A
        LD    C,18H         ; 24
LBL29:  CALL  SUB6
        DEC   E
        JP    NZ,LBL29
        CALL  printNewLine
        POP   AF
        POP   BC
        POP   DE
        POP   HL
        JP    LBL25
LBL30:  POP   HL
        INC   HL
        JP    LBL25
SUB12:  LD    A,(HL)
        PUSH  BC
        CALL  mon2_printHexByte
        CALL  printSpace
        POP   BC
        RET

;----------------------------------------------------------------------------

Dir_N:  LD    A,(HL)
        CP    C
        JP    Z,LBL32
        CALL  printHexWordHL
        LD    A,(HL)
        PUSH  BC
        CALL  mon2_printHexByte
        CALL  WaitClsKey
        CALL  printNewLine
        POP   BC
LBL32:  CALL  bios_cmp_hl_de
        RET   Z
        INC   HL
        JP    Dir_N

;----------------------------------------------------------------------------
; Директива F - заполнить блок памяти байтом

Dir_F:  LD    (HL),C
        CALL  bios_cmp_hl_de
        RET   Z
        INC   HL
        JP    Dir_F

;----------------------------------------------------------------------------
; Директива T - копирование блока памяти на новый адрес
; Перекрывающиеся области памяти будут испорчены

Dir_T:  LD    A,(HL)
        LD    (BC),A
        CALL  bios_cmp_hl_de
        RET   Z
        INC   HL
        INC   BC
        JP    Dir_T

;----------------------------------------------------------------------------
; Директива C - сравнение двух блоков памяти

Dir_C:  LD    A,(BC)
        CP    (HL)
        JP    Z,LBL36
        PUSH  BC
        CALL  printHexWordHL
        LD    A,(HL)
        CALL  mon2_printHexByte
        CALL  printSpace
        POP   BC
        PUSH  BC
        LD    A,(BC)
        CALL  mon2_printHexByte
        CALL  WaitClsKey
        CALL  printNewLine
        POP   BC
LBL36:  CALL  bios_cmp_hl_de
        RET   Z
        INC   HL
        INC   BC
        JP    Dir_C

;----------------------------------------------------------------------------
; Печать слова в HEX формате из HL, обрамляется пробелами

printHexWordHL:
        PUSH  BC
        CALL  printSpace
printHexWordHL_noSpace:
        LD    A,H
        CALL  mon2_printHexByte
        LD    A,L
        CALL  mon2_printHexByte
        CALL  printSpace
        POP   BC
        RET

;----------------------------------------------------------------------------
; Расчет и печать контрольной суммы
; вход:
;   hl = начальный адрес
;   de = конечный адрес

Dir_K:
        CALL  mon2_calcCS
        PUSH  BC
        POP   HL    ; hl = bc
        CALL  printHexWordHL
        RET

;----------------------------------------------------------------------------
; Директива L - дамп блока памяти в текстовом виде

Dir_L:  CALL  printHexWordHL
        LD    B,10H         ; 16
LBL40:  LD    C,(HL)
        LD    A,(vars1.ThirdParam)
        OR    A
        JP    Z,LBL41
        AND   C
        LD    C,A
LBL41:  LD    A,C
        CP    20H           ; 32 ' '
        JP    C,LBL42
        CP    7FH           ; 127
        JP    C,LBL43
LBL42:  LD    C,2EH         ; 46 '.'
LBL43:  CALL  mon2_printChar
        CALL  printSpace
        CALL  bios_cmp_hl_de
        RET   Z
        DEC   B
        INC   HL
        JP    NZ,LBL40
        CALL  WaitClsKey
        CALL  printNewLine
        JP    Dir_L

;----------------------------------------------------------------------------
; Анализ строки в буфере и разбивка на компоненты:
; A = буква директивы
; HL = первый HEX параметр
; DE = второй HEX параметр
; BC = третий HEX параметр

Tokenizer:
        ; читаем букву директивы
        LD    BC,INPUT_BUF
        LD    A,(BC)

        ; сохраняем букву директивы
        LD    (vars1.Directive),A
        INC   BC

SUB15:  ; читаем и сохраняем три HEX параметра
        CALL  ReadHexWord
        LD    (vars1.FirstParam),HL
        CALL  ReadHexWord
        LD    (vars1.SecondParam),HL
        CALL  ReadHexWord
        LD    (vars1.ThirdParam),HL

        LD    L,C
        LD    H,B
        LD    (vars1.V_8F87),HL

        ; помещаем три HEX параметра в регитсры HL, DE, BC
        LD    HL,(vars1.ThirdParam)
        LD    C,L
        LD    B,H
        LD    HL,(vars1.SecondParam)
        EX    DE,HL
        LD    HL,(vars1.FirstParam)

        ; и букву директиры - в регистр A
        LD    A,(vars1.Directive)

        ; выходим
        RET

;----------------------------------------------------------------------------
; Преобразование строки по адресу BC в HEX число в HL

ReadHexWord:
        LD    HL,0000H
LBL44:  LD    A,(BC)
        CP    0DH           ; 13
        RET   Z
        CP    2CH           ; 44 ','
        JP    Z,LBL45
        CALL  SUB17
        ADD   HL,HL
        ADD   HL,HL
        ADD   HL,HL
        ADD   HL,HL
        ADD   A,L
        LD    L,A
        INC   BC
        JP    LBL44
LBL45:  INC   BC
        RET

;----------------------------------------------------------------------------

SUB17:  SUB   '0'
        CP    0Ah
        RET   C
        SUB   11h
        CP    06h
        JP    NC,TypeErrorRestart
        ADD   A,10
        RET

;----------------------------------------------------------------------------

SUB18:  PUSH  BC
        LD    C,19h
        CALL  mon2_printChar
        POP   BC
        RET

;----------------------------------------------------------------------------

SUB19:  PUSH  HL
        LD    HL,INPUT_BUF
        JP    LBL53

;----------------------------------------------------------------------------
; Печать перевода строки (0Ah, 0Dh)

printNewLine:
        LD    C,0AH
        CALL  mon2_printChar
        LD    C,0DH
        CALL  mon2_printChar
        RET

;----------------------------------------------------------------------------
; Русский текст в кодировке КОИ-7

txtSpaceQuestMark:  DB  "  ?",00H
txtNewLine:         DB  0AH,0DH,00H
txtHomeRight11Dot:  DB  0DH,18H,18H,18H,18H,18H,18H,18H,18H,18H,18H,18H,2EH,00H
txtFileQuestMark:   DB  "FILE? ",00H
                    DB  0AH,"FILE: ",00H

;----------------------------------------------------------------------------
; Директива M - редактирование блока памяти

Dir_M:  CALL  SUB18
LBL47:  CALL  printNewLine
        CALL  printHexWordHL
        LD    A,(HL)
        CALL  mon2_printHexByte
        CALL  printSpace
        CALL  mon2_getch
        CALL  RestartIfCls
        CP    1AH           ; кнопка вниз
        JP    Z,LBL48
        CP    08H           ; кнопка влева
        JP    Z,LBL49
        CALL  SUB21
        LD    (HL),A
LBL48:  INC   HL
        JP    LBL47
LBL49:  DEC   HL
        JP    LBL47
SUB21:  PUSH  BC
        JP    LBL50
        PUSH  BC
        CALL  mon2_getch
LBL50:  CALL  SUB22
        RLCA
        RLCA
        RLCA
        RLCA
        LD    B,A
        CALL  mon2_getch
        CALL  SUB22
        OR    B
        POP   BC
        RET

SUB22:  CP    20H           ; 32 ' '; [2]
        JP    C,RestartNoCls
        CP    3AH           ; 58 ':'
        JP    C,LBL51
        AND   5FH           ; 95 '_'
        CP    41H           ; 65 'A'
        JP    C,TypeErrorRestart
        CP    47H           ; 71 'G'
        JP    NC,TypeErrorRestart
        PUSH  AF
        LD    C,A
        CALL  mon2_printChar
        POP   AF
        SUB   37H           ; 55 '7'
        RET
LBL51:  OR    10H           ; 16
        PUSH  AF
        LD    C,A
        CALL  mon2_printChar
        POP   AF
        SUB   30H           ; 48 '0'
        RET

;----------------------------------------------------------------------------
; Печать знака вопроса и перезапуск

TypeErrorRestart:
        LD    HL,txtSpaceQuestMark
        CALL  bios_printStringOld
        JP    RestartNoCls

;----------------------------------------------------------------------------

LBL53:  PUSH  BC
        PUSH  DE
        LD    DE,vars1.Directive
        CALL  SUB28
        POP   DE
        POP   BC
        POP   HL
        RET

;----------------------------------------------------------------------------
; Печать пробела

printSpace:
        LD    C,20H
        CALL  mon2_printChar
        RET

;----------------------------------------------------------------------------
; Ожидание нажатия клавиши СТР

WaitClsKey:
        CALL  mon2_keyScan
        CP    1FH
        RET   NZ
        CALL  mon2_getch
        ; продолжение в RestartIfCls

;----------------------------------------------------------------------------
; Перезапуск Монитора, если A == 1Fh

RestartIfCls:
        CP    1FH
        RET   NZ
        JP    RestartNoCls

;----------------------------------------------------------------------------
; Директива D - дамп блока памяти в HEX виде

Dir_D:  CALL  mon2_printHexWord
        LD    A,L
        AND   0FH
        LD    C,A
        CPL
        AND   0FH
        INC   A
        LD    B,A
        LD    A,C
        ADD   A,A
        ADD   A,A
        ADD   A,A
        ADD   A,C
        ADD   A,0FH
        LD    (bios_vars.cursorX),A
LBL55:  LD    A,(HL)
        CALL  mon2_printHexByte
        LD    A,B
        CP    09H
        JP    NZ,LBL56
        LD    C,2DH         ; печать символа '-'
        CALL  mon2_printChar
        JP    LBL57
LBL56:  CALL  printSpace
LBL57:  CALL  bios_cmp_hl_de
        RET   Z
        DEC   B
        INC   HL
        JP    NZ,LBL55
        CALL  WaitClsKey
        CALL  printNewLine
        JP    Dir_D
        LD    E,A
        RRCA
        RRCA
        RRCA
        RRCA
        CALL  SUB26
        LD    D,A
        LD    A,E
        CALL  SUB26
        LD    E,A
        RET
SUB26:  AND   0FH           ; 15; [2]
        CP    0AH           ; 10
        JP    C,LBL58
        ADD   A,07H         ; 7
LBL58:  ADD   A,30H         ; 48 '0'
        RET

;----------------------------------------------------------------------------

txtRegisters:
        DB    "A=",0
        DB    "F=",0
        DB    "B=",0
        DB    "C=",0
        DB    "D=",0
        DB    "E=",0
        DB    "H=",0
        DB    "L=",0
        DB    0AH,0DH," M(HL)=",0
        DB    "PC=",0
        DB    "SP=",0

;----------------------------------------------------------------------------

SUB27:  PUSH  AF            ; [2]
LBL59:  LD    A,(vars2.V_F0E6)
        AND   0CH           ; 12
        JP    NZ,LBL59
        LD    A,C
        LD    (vars2.V_F0E5),A
        LD    A,(vars2.V_F0E6)
        OR    20H           ; 32 ' '
        LD    (vars2.V_F0E6),A
LBL60:  LD    A,(vars2.V_F0E6)
        AND   04H           ; 4
        JP    Z,LBL60
        XOR   A
        LD    (vars2.V_F0E6),A
        POP   AF
        RET
        PUSH  HL
LBL61:  LD    A,(HL)
        OR    A
        JP    Z,LBL62
        LD    C,A
        CALL  SUB27
        INC   HL
        JP    LBL61
LBL62:  POP   HL
        RET
        PUSH  AF
        LD    A,91H         ; 145
        LD    (vars2.V_F0E7),A
        LD    A,(vars2.V_F0E6)
        OR    10H           ; 16
        LD    (vars2.V_F0E6),A
        AND   0EFH          ; 239
        LD    (vars2.V_F0E6),A
        LD    C,0FH         ; 15
        CALL  SUB27
        POP   AF
        RET
LBL63:  LD    (bios_vars.cursorCfg),A
        CALL  bios_drawCursorOld
        POP   AF
        LD    (bios_vars.cursorCfg),A
        LD    C,18H         ; 24
        POP   AF
        JP    mon2_printChar
;----------------------------------------------------------------------------

SUB28:  LD    B,H
        LD    C,L
LBL64:  CALL  mon2_getch
        CP    08H           ; клавиша влево
        JP    Z,LBL65
        CP    0DH           ; клавиша Enter
        JP    Z,LBL67
        LD    (vars1.V_8F95),A
        CALL  bios_cmp_hl_de
        JP    Z,LBL64
        LD    A,(vars1.V_8F95)
        LD    (HL),A
        PUSH  BC
        LD    C,A
        CALL  mon2_printChar
        POP   BC
        INC   HL
        JP    LBL64

;----------------------------------------------------------------------------

LBL65:  LD    A,H
        CP    B
        JP    NZ,LBL66
        LD    A,L
        CP    C
        JP    Z,LBL64
LBL66:  DEC   HL
        PUSH  BC
        LD    C,08H         ; 8
        CALL  mon2_printChar
        LD    C,20H         ; 32 ' '
        CALL  mon2_printChar
        LD    C,08H         ; 8
        CALL  mon2_printChar
        POP   BC
        JP    LBL64
LBL67:
        LD    (HL),A
        RET
        PUSH  HL
        LD    HL,0FFE3H   ; 65507
        LD    (HL),0DH    ; 13
        LD    (HL),0CH    ; 12
        POP   HL
        RET

;----------------------------------------------------------------------------
; Таблица переходов F800h
;----------------------------------------------------------------------------
    ORG_PAD0 0F800h


mon2_restart:       JP    RestartNoCls          ; F800
mon2_getch:         JP    bios_getchOld         ; F803
                    JP    bios_tapeReadOld      ; F806
mon2_printChar:     JP    bios_printCharOld     ; F809
                    JP    bios_tapeWriteOld     ; F80C
                    JP    bios_printer          ; F80F
                    JP    setMIfKeyPressed      ; F812
mon2_printHexByte:  JP    bios_printHexByte     ; F815
mon2_printString:   JP    bios_printStringOld   ; F818
mon2_keyScan:       JP    bios_keyScanOld       ; F81B
                    JP    bios_getCursorPos     ; F81E
                    RET                         ; F821 - не используется
                    NOP
                    NOP
mon2_tapeLoad:      JP    bios_tapeLoad         ; F824
mon2_tapeSave:      JP    bios_tapeSave         ; F827
mon2_calcCS:        JP    bios_calcCS           ; F82A
                    JP    bios_beep_Old         ; F82D
                    JP    bios_getMemTop        ; F830
                    JP    bios_setMemTop        ; F833
mon2_printHexWord:  PUSH  BC                    ; F836 - печать слова из HL в HEX формате, дополняется пробелом
                    JP    printHexWordHL_noSpace

;----------------------------------------------------------------------------

    END
