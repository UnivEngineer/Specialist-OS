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

; Собственные переменные
    STRUCT MON2_VARS_1
V_8F80:     DS    1
V_8F81:     DS    2
V_8F83:     DS    2
V_8F85:     DS    2
V_8F87:     DS    2
V_8F89:     DS    1
V_8F8A:     DS    1
V_8F8B:     DS    1
V_8F8C:     DS    2
V_8F8E:     DS    2
V_8F90:     DS    1
V_8F91:     DS    4
V_8F95:     DS    1
V_8F96:     DS    1
V_8F97:     DS    2
V_8F99:     DS    1
V_8F9A:     DS    3
V_8F9D:     DS    2
V_8F9F:     DS    2
V_8FA1:     DS    2
V_8FA3:     DS    2
V_8FA5:     DS    2
V_8FA7:     DS    4
V_8FAB:     DS    2
    ENDS

    STRUCT MON2_VARS_2
V_F0E0:     DS    5
V_F0E5:     DS    1
V_F0E6:     DS    1
V_F0E7:     DS    1
    ENDS

    STRUCT MON2_VARS_3
; похоже на старый дескриптор файла
V_F900:     DS    10    ; имя (6), расширение (3), атрибуты (1)
V_F90A:     DS    2     ; адрес загрузки
V_F90C:     DS    2     ; размер-1
    ENDS

; Адреса блоков переменных
vars1 MON2_VARS_1 = 08F80H
vars2 MON2_VARS_2 = 0F0E0H
vars3 MON2_VARS_3 = 0F900H


; Начало программы
    ORG   0F100h

        ; Начинаем с очистки экрана
        LD    C,1FH ; rjl символа очистки экрана
        CALL  bios_printChar

        ; Рестарт без очистки экрана
restartNoCls:
        LD    SP,0FFBFH
        LD    HL,mon2_restart           ; установить обработчик ошибки магнитофона
        LD    (bios_vars.tapeError),HL  ; на рестарт монитора
        LD    HL,7EFFH    ; 32511
        LD    (vars1.V_8FAB),HL
        CALL  SUB1
        JP    restartNoCls

SUB1:   LD    HL,txtNewLine             ; 62970
        CALL  bios_printStringOld
        LD    E,02H         ; 2
        CALL  bios_fileGetSetDrive
        ADD   A,41H         ; 65 'A'
        LD    C,A
        CALL  bios_printChar
        LD    C,3EH         ; 62 '>'
        CALL  bios_printChar
        CALL  SUB2
        CALL  SUB19
        PUSH  HL
        PUSH  DE
        PUSH  BC
        LD    E,02H         ; 2
        CALL  bios_fileGetSetDrive
        PUSH  AF
        LD    HL,8F60H    ; 36704
        LD    DE,vars3.V_F900   ; 63744
        CALL  bios_fileNamePrepare
        LD    E,02H         ; 2
        CALL  bios_fileGetSetDrive
        LD    E,A
        POP   AF
        CP    E
        POP   BC
        POP   DE
        POP   HL
        RET   NZ
        CALL  printNewLine
        CALL  SUB14
        CP    44H           ; 68 'D'
        JP    Z,LBL54
        CP    4DH           ; 77 'M'
        JP    Z,LBL46
        CP    4CH           ; 76 'L'
        JP    Z,LBL39
        CP    4BH           ; 75 'K'
        JP    Z,calcAndPrintCRC
        CP    54H           ; 84 'T'
        JP    Z,LBL34
        CP    58H           ; 88 'X'
        JP    Z,LBL15
        CP    57H           ; 87 'W'
        JP    Z,mon2_tapeSave
        CP    52H           ; 82 'R'
        JP    Z,LBL12
        CP    43H           ; 67 'C'
        JP    Z,LBL35
        CP    48H           ; 72 'H'
        JP    Z,LBL14
        CP    4EH           ; 78 'N'
        JP    Z,LBL31
        CP    47H           ; 71 'G'
        JP    Z,LBL18
        CP    46H           ; 70 'F'
        JP    Z,LBL33
        CP    53H           ; 83 'S'
        JP    Z,LBL21
        CP    4AH           ; 74 'J'
        JP    Z,LBL2
        CP    3FH           ; 63 '?'
        JP    Z,Dir_DirDisk
        CP    42H           ; 66 'B'
        JP    Z,LBL5
        CP    41H           ; 65 'A'
        JP    Z,LBL7
        CP    56H           ; 86 'V'
        JP    Z,LBL8
        CP    55H           ; 85 'U'
        JP    Z,LBL9
        CP    59H           ; 89 'Y'
        JP    Z,LBL10
        CP    51H           ; 81 'Q'
        JP    Z,Dir_GetFileAttr
        JP    LBL52
LBL2:   CALL  bios_keyCheck             ; [2]
        INC   A
        JP    Z,bios_reboot
        JP    LBL2
SUB2:   LD    HL,8F60H    ; 36704
        LD    (HL),00H
        INC   HL
        LD    (HL),00H
        RET

        ; Директива ? - вывод каталога диска
Dir_DirDisk:
        LD    HL,vars3.V_F900   ; буфер
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
        LD    B,06H             ; печатаем 6 символов имени файла
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
        CALL  printHexWordHL   ; печатаем начальный адрес загрузки файла
        ADD   HL,DE
        CALL  printHexWordHL   ; печатаем конечынй адрес загрузки файла
        POP   HL                ; HL = указатель дескриптора
        LD    A,L
        AND   0F0H
        LD    L,A               ; Обнуляем младшие 4 бита HL - переход на начало текущего дескриптора файла
        LD    DE,0010H
        ADD   HL,DE             ; HL += 16 - переход на следующий дескриптор файла
        CALL  printNewLine
        JP    DirDiskLoop

        ; Печать строки по адресу HL длиной B
printStringB:
        LD    C,(HL)
        CALL  bios_printChar
        INC   HL
        DEC   B
        JP    NZ,printStringB
        RET

LBL5:   LD    (vars3.V_F90A),HL
        EX    DE,HL
        LD    A,D
        CPL
        LD    D,A
        LD    A,E
        CPL
        LD    E,A
        INC   DE
        ADD   HL,DE
        LD    (vars3.V_F90C),HL
        CALL  EnterFileName
        CALL  bios_fileCreate
        LD    HL,txtSmallDisk             ; 62041
        CALL  C,SUB4
        RET
SUB4:   OR    A
        JP    Z,LBL6
        CALL  bios_printString
        RET
LBL6:   LD    HL,txtSmallDir             ; 62054
        CALL  bios_printString
        RET

        ; Русский текст в кодировке КОИ-8
txtSmallDisk:   DB  0AH,0DH,"mal disk !",00H
txtSmallDir:    DB  0AH,0DH,"mal DIR !", 00H
txtNoFile:      DB  0AH,0DH,"net fajla ",00H

LBL7:   PUSH  HL
        CALL  EnterFileName
        POP   DE
        LD    C,01H
        CALL  bios_fileGetSetAddr
        LD    HL,txtNoFile             ; 62066
        CALL  C,bios_printString
        RET
LBL8:   PUSH  HL
        CALL  EnterFileName
        POP   DE
        PUSH  DE
        CALL  bios_fileLoad2
        PUSH  AF
        LD    HL,txtNoFile             ; 62066
        CALL  C,bios_printString
        POP   AF
        POP   DE
        RET   C
        PUSH  DE
        LD    HL,vars3.V_F900   ; 63744
        CALL  bios_fileLoadInfo
        LD    HL,(vars3.V_F90C)
        EX    DE,HL
        POP   HL
        CALL  printHexWordHL
        ADD   HL,DE
        CALL  printHexWordHL
        RET
LBL9:   CALL  EnterFileName
        CALL  bios_fileLoad
        PUSH  AF
        LD    HL,txtNoFile             ; 62066
        CALL  C,bios_printString
        CALL  printSpace
        POP   AF
        RET   C
        LD    HL,vars3.V_F900   ; 63744
        CALL  bios_fileLoadInfo
        LD    HL,(vars3.V_F90A)
        PUSH  HL
        CALL  printHexWordHL
        POP   DE
        LD    HL,(vars3.V_F90C)
        ADD   HL,DE
        CALL  printHexWordHL
        RET

LBL10:  PUSH  HL
        CALL  EnterFileName         ; ввод имени файла
        LD    C,01H                 ; режим для фаункции bios_fileGetSetAttr - установка байта атрибутов файла
        EX    (SP),HL
        LD    A,L
        POP   HL
        CALL  bios_fileGetSetAttr   ; установка байта атрибутов файла
        LD    HL,txtNoFile
        CALL  C,bios_printString    ; если файл не найден - вывести сообщение
        RET

        ; Директива Q - вывод атрибутов файла
Dir_GetFileAttr:
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

        ; Запрос имени файла
        ; на выходе:
        ;   HL = адрем буфера с подготовленным именем файла
EnterFileName:
        LD    HL,txtFileQuestMark   ; печать приглашеиня ввести имя файла
        CALL  bios_printString
        LD    HL,8F60H              ; буфер для ввода строки
        LD    DE,8F6DH
        CALL  bios_input
        LD    DE,vars3.V_F900       ; буфер для подготовленного имени файла
        CALL  bios_fileNamePrepare  ; подготавливаем имя файла
        EX    DE,HL                 ; HL = подготовленное имя файла
        RET

LBL12:  CALL  SUB34
        PUSH  BC
        PUSH  DE
        PUSH  HL
        CALL  bios_calcCS
        POP   HL
        CALL  printHexWordHL
        POP   HL
        CALL  printHexWordHL
        POP   HL
        LD    D,B
        LD    E,C
        CALL  bios_cmp_hl_de
        JP    NZ,LBL13
        CALL  printHexWordHL
        RET
LBL13:  CALL  printSpace
        LD    C,3FH         ; 63 '?'
        CALL  bios_printChar
        RET
SUB6: PUSH  AF
        LD    A,(bios_vars.cursorCfg)
        PUSH  AF
        LD    A,11H         ; 17
        JP    LBL63
        NOP
        NOP
LBL14:  PUSH  HL
        ADD   HL,DE
        CALL  printHexWordHL
        CALL  printSpace
        LD    A,E
        CPL
        LD    E,A
        LD    A,D
        CPL
        LD    D,A
        INC   DE
        POP   HL
        ADD   HL,DE
        CALL  printHexWordHL
        RET
LBL15:  LD    HL,vars1.V_8FA3+1 ; 36772
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

        ; Если нажата клавиша, записать 0FFh по адресу (HL)
setMIfKeyPressed:
        CALL  bios_keyScan
        INC   A
        RET   Z
        LD    (HL), 0FFh
        RET

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
LBL18:  LD    A,E
        OR    A
        JP    NZ,LBL19
        LD    A,D
        OR    A
        JP    Z,LBL20
LBL19:  PUSH  HL
        EX    DE,HL
        LD    (vars1.V_8F97),HL
        LD    A,(HL)
        LD    (HL),0FFH   ; 255
        LD    (vars1.V_8F99),A
        LD    HL,0038H    ; 56
        LD    DE,vars1.V_8F9A   ; 36762
        LD    BC,REF4             ; 62455
        CALL  SUB11
        LD    (HL),0C3H   ; 195
        CALL  SUB10
        CALL  SUB10
        LD    (HL),B
        DEC   HL
        LD    (HL),C
        POP   HL
LBL20:  CALL  SUB9
        JP    restartNoCls
SUB9:   JP    (HL)
SUB10:  INC   HL            ; [4]
        INC   DE
SUB11:  LD    A,(HL)                ; [2]
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
        LD    HL,vars1.V_8F9A   ; 36762
        LD    DE,0038H    ; 56
        CALL  SUB11
        CALL  SUB10
        CALL  SUB10
        JP    restartNoCls
LBL21:  PUSH  HL
        PUSH  BC
        LD    HL,8F60H    ; 36704
        LD    C,00H
LBL22:  LD    A,(HL)
        CP    2CH           ; 44 ','
        JP    NZ,LBL23
        INC   C
LBL23:  INC   HL
        CP    0DH           ; 13
        JP    NZ,LBL22
        DEC   C
        LD    A,C
        LD    (vars1.V_8F89),A
        CP    01H           ; 1
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
        LD    HL,vars1.V_8F8C   ; 36748
        LD    (HL),E
        INC   HL
        LD    (HL),C
        POP   BC
        POP   DE
        POP   HL
LBL24:  LD    A,C
        LD    (vars1.V_8F8A),A
LBL25:  PUSH  HL            ; [2]
        LD    HL,vars1.V_8F8A   ; 36746
        LD    (vars1.V_8F8E),HL
        POP   HL
        CALL  bios_cmp_hl_de
        RET   Z
        CALL  SUB24
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
LBL31:  LD    A,(HL)                ; [2]
        CP    C
        JP    Z,LBL32
        CALL  printHexWordHL
        LD    A,(HL)
        PUSH  BC
        CALL  mon2_printHexByte
        CALL  SUB24
        CALL  printNewLine
        POP   BC
LBL32:  CALL  bios_cmp_hl_de
        RET   Z
        INC   HL
        JP    LBL31
LBL33:  LD    (HL),C                ; [2]
        CALL  bios_cmp_hl_de
        RET   Z
        INC   HL
        JP    LBL33
LBL34:  LD    A,(HL)                ; [2]
        LD    (BC),A
        CALL  bios_cmp_hl_de
        RET   Z
        INC   HL
        INC   BC
        JP    LBL34
LBL35:  LD    A,(BC)                ; [2]
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
        CALL  SUB24
        CALL  printNewLine
        POP   BC
LBL36:  CALL  bios_cmp_hl_de
        RET   Z
        INC   HL
        INC   BC
        JP    LBL35

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

        ; Расчет и печать контрольной суммы
        ; вход:
        ;   hl = начальный адрес
        ;   de = конечный адрес
calcAndPrintCRC:
        CALL  mon2_calcCS
        PUSH  BC
        POP   HL    ; hl = bc
        CALL  printHexWordHL
        RET

LBL39:  CALL  printHexWordHL         ; [2]
        LD    B,10H         ; 16
LBL40:  LD    C,(HL)
        LD    A,(vars1.V_8F85)
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
        CALL  SUB24
        CALL  printNewLine
        JP    LBL39
SUB14:  LD    BC,8F60H    ; 36704
        LD    A,(BC)
        LD    (vars1.V_8F80),A
        INC   BC
SUB15:  CALL  SUB16
        LD    (vars1.V_8F81),HL
        CALL  SUB16
        LD    (vars1.V_8F83),HL
        CALL  SUB16
        LD    (vars1.V_8F85),HL
        LD    L,C
        LD    H,B
        LD    (vars1.V_8F87),HL
        LD    HL,(vars1.V_8F85)
        LD    C,L
        LD    B,H
        LD    HL,(vars1.V_8F83)
        EX    DE,HL
        LD    HL,(vars1.V_8F81)
        LD    A,(vars1.V_8F80)
        RET
SUB16:  LD    HL,0000H    ; [3]
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
SUB17:  SUB   30H           ; 48 '0'
        CP    0AH           ; 10
        RET   C
        SUB   11H           ; 17
        CP    06H           ; 6
        JP    NC,LBL52
        ADD   A,0AH         ; 10
        RET
SUB18:  PUSH  BC
        LD    C,19H         ; 25
        CALL  mon2_printChar
        POP   BC
        RET
SUB19:  PUSH  HL
        LD    HL,8F60H    ; 36704
        JP    LBL53

        ; Печать перевода строки (0Ah, 0Dh)
printNewLine:
        LD    C,0AH
        CALL  mon2_printChar
        LD    C,0DH
        CALL  mon2_printChar
        RET

        ; Русский текст в кодировке КОИ-8
txtSpaceQuestMark:  DB  "  ?",00H
txtNewLine:         DB  0AH,0DH,00H
txtHomeRight11Dot:  DB  0DH,18H,18H,18H,18H,18H,18H,18H,18H,18H,18H,18H,2EH,00H
txtFileQuestMark:   DB  "fajl ? ",00H
                    DB  0AH,"fajl: ",00H

LBL46:  CALL  SUB18
LBL47:  CALL  printNewLine         ; [2]
        CALL  printHexWordHL
        LD    A,(HL)
        CALL  mon2_printHexByte
        CALL  printSpace
        CALL  mon2_getch
        CALL  SUB25
        CP    1AH           ; 26
        JP    Z,LBL48
        CP    08H           ; 8
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
        JP    C,restartNoCls
        CP    3AH           ; 58 ':'
        JP    C,LBL51
        AND   5FH           ; 95 '_'
        CP    41H           ; 65 'A'
        JP    C,LBL52
        CP    47H           ; 71 'G'
        JP    NC,LBL52
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
LBL52:  LD    HL,txtSpaceQuestMark             ; 62966; [4]
        CALL  bios_printStringOld
        JP    restartNoCls
LBL53:  PUSH  BC
        PUSH  DE
        LD    DE,vars1.V_8F80   ; 36736
        CALL  SUB28
        POP   DE
        POP   BC
        POP   HL
        RET

        ; Печать пробела
printSpace:
        LD    C,20H
        CALL  mon2_printChar
        RET

SUB24:  CALL  mon2_keyScan         ; [5]
        CP    1FH           ; 31
        RET   NZ
        CALL  mon2_getch
SUB25:  CP    1FH           ; 31
        RET   NZ
        JP    restartNoCls
LBL54:  CALL  mon2_printHexWord
        LD    A,L
        AND   0FH           ; 15
        LD    C,A
        CPL
        AND   0FH           ; 15
        INC   A
        LD    B,A
        LD    A,C
        ADD   A,A
        ADD   A,A
        ADD   A,A
        ADD   A,C
        ADD   A,0FH         ; 15
        LD    (bios_vars.cursorX),A
LBL55:  LD    A,(HL)
        CALL  mon2_printHexByte
        LD    A,B
        CP    09H           ; 9
        JP    NZ,LBL56
        LD    C,2DH         ; 45 '-'
        CALL  mon2_printChar
        JP    LBL57
LBL56:  CALL  printSpace
LBL57:  CALL  bios_cmp_hl_de
        RET   Z
        DEC   B
        INC   HL
        JP    NZ,LBL55
        CALL  SUB24
        CALL  printNewLine
        JP    LBL54
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
SUB28:  LD    B,H
        LD    C,L
LBL64:  CALL  mon2_getch         ; [4]
        CP    08H           ; 8
        JP    Z,LBL65
        CP    0DH           ; 13
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


mon2_restart:       JP    restartNoCls          ; F800
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
SUB34:              JP    bios_tapeLoad         ; F824
mon2_tapeSave:      JP    bios_tapeSave         ; F827
mon2_calcCS:        JP    bios_calcCS           ; F82A
                    JP    bios_beep_Old         ; F82D
                    JP    bios_getMemTop        ; F830
                    JP    bios_setMemTop        ; F833
mon2_printHexWord:  PUSH  BC                    ; F836 - печать слова из HL в HEX формате, дополняется пробелом
                    JP    printHexWordHL_noSpace

    END
