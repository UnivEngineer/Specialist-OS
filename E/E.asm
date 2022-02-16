;----------------------------------------------------------------------------
; MXOS
; E.COM - текстовой редактор
;
; 2022-02-03 Дизассемблировано SpaceEngineer
;----------------------------------------------------------------------------

    INCLUDE "../include/mxos.inc"

; Компиляция оригинального редаткора (если 1)
STANDARD_EDITOR = 0

; Используемые подпрограммы DOS.SYS:
; bios_beep_Old         = 0C170h
; bios_delay_b          = 0C190h
; bios_cmp_hl_de        = 0C427h
; bios_memcpy_bc_hl     = 0C42Dh
; bios_reboot           = 0C800h
; bios_getch            = 0C803h
; bios_tapeRead         = 0C806h
; bios_printChar        = 0C809h
; bios_tapeWrite        = 0C80Ch
; bios_printString      = 0C818h
; bios_calcCS           = 0C82Ah
; bios_fileCreate       = 0C845h
; bios_fileLoadInfo     = 0C851h
; bios_fileNamePrepare  = 0C85Ah
; bios_fileLoad2        = 0C866h

; Используемые переменные DOS.SYS:
; bios_vars.lastKey     = 08FF0h
; bios_vars.tapeInverse = 08FF3h
; bios_vars.cursorDelay = 08FF4h
; bios_vars.inverse     = 08FFAh
; bios_vars.cursorY     = 08FFCh

; Используемые порты устройств:
; IO_KEYB_A = 0FFE0h
; IO_KEYB_B = 0FFE1h

;----------------------------------------------------------------------------

; Цвета

COLOR_TEXT      = 0B1h
COLOR_STATUSBAR = 071h

; Когда цвет включен, макрос вставляет такой код
    MACRO COLOR x
        IF STANDARD_EDITOR==0
            ld a, x
            ld (IO_COLOR), a
        ENDIF
    ENDM

;----------------------------------------------------------------------------

; Собственные переменные
    STRUCT EDITOR_VARIABLES
buffer  BLOCK  10, 0
last    BLOCK  1,  0
    ENDS

; Блок переменных редактора начинается с адреса 8F80H
editor_vars EDITOR_VARIABLES = 8F80H

;----------------------------------------------------------------------------

; Начало программы
    IF NEW_MEMORY_MAP && !STANDARD_EDITOR
        ORG 0E800h
    ELSE
        ORG 0D000h
    ENDIF

;----------------------------------------------------------------------------

; Буферы в конце программы
BUFFER1 = $ + 0F0Ah ; 0DF0AH
BUFFER2 = $ + 0F00h ; 0DF00H

;----------------------------------------------------------------------------

SMC1:
        JP    LBL145    ; адрес этого перехода изменяется п/п LBL3
        JP    LBL103

        ; Что-то непонятное, границы буфера текста?
VAR01:  DB    0
VAR02:  DB    18h
VAR03:  DB    0
VAR04:  DB    85h

LBL2:   LD    HL,REF2
        CALL  bios_printString

        ; Похоже на обработчик ошибки ввода/вывода с ленты
LBL3:   ; ??? Считаем контрольную сумму блока 7000-8EFF
        LD    HL,7000H
        LD    DE,8F00H
        CALL  bios_calcCS
        ; Суровая самомодификация кода:
        ; Меняем адрес в первой инстукции JP в начале редактора на LBL4
SMC6:   LD    HL, LBL4
        LD    (SMC1+1),HL

        ; И прыгаем на LBL4
        JP    LBL4

REF2:   DB    0AH,"          EDITOR VERSION 4.1, (C) OM"
REF3:   DB    "S"
REF4:   DB    "K"
REF5:   DB    " "
REF6:   DB    "KSOFT ",27H,"92",2EH,00H
VAR1:   DW    0000H
VAR2:   DW    0000H
VAR3:   DW    0000H
VAR4:   DB    00H
VAR5:   DB    00H
VAR6:   DW    0000H
VAR7:   DW    0000H
VAR8:   DW    0000H
REF7:   DB    04H,05H,08H
VAR9:   DB    00H
REF8:   DB    1FH,0AH,"  OUT OF MEMORY",2EH,00H
REF9:   DB    1FH
REF10:  DB    0AH," FILE: ",00H
REF11:  DB    08H,20H,08H,00H           ; <-, ' ', <-
REF12:  DB    0AH," MISTAKE",2EH,00H
REF13:  DB    "SEARCH: "
REF14:  DB    00H,"ALL V33",00H,"RLN]N",00H,"y"

txtNew:     DB    1FH,0AH," NEW ?",00H
txtIns:     DB    "INSERT   ",00H
txtOvr:     DB    "OVERWRITE",00H
txtLine:    DB    "LINE",00H
txtCol:     DB    "COL",00H

LBL4:   ; Читаем указатель стека в hl
        LD    HL,0000H
        ADD   HL,SP
        ; И записываем его в код п/п SMC4
        LD    (SMC4+1),HL

        COLOR COLOR_TEXT

LBL5:   LD    A,10H         ; 16
        LD    (editor_vars.last),A
        XOR   A
        LD    (VAR4),A
        LD    H,A
        LD    L,A
        LD    (VAR6),HL
        LD    (bios_vars.inverse),HL
        LD    HL,0001H    ; 1
        LD    (VAR8),HL
        LD    HL,(VAR01)
        LD    (VAR1),HL
        LD    (VAR3),HL
        LD    E,L
        LD    D,H
LBL6:   LD    A,(HL)
        OR    A
        JP    NZ,LBL7
        DEC   A
        LD    (HL),A
LBL7:   INC   A
        JP    Z,LBL8
        INC   HL
        JP    LBL6
LBL8:   CALL  SUB38
        JP    NZ,LBL9
        LD    (HL),0DH    ; 13
        INC   HL
        LD    (HL),0FFH   ; 255
LBL9:   LD    (VAR2),HL
LBL10:  LD    C,1FH         ; 31
        CALL  bios_printChar
        CALL  SUB1
REF21:  LD    HL,REF21    ; 53535
        PUSH  HL
        LD    HL,(bios_vars.cursorY)
        PUSH  HL
        CALL  SUB56
        LD    HL,0000H
        CALL  SUB14
        EX    DE,HL
        LD    HL,8CF9H    ; 36089
        LD    (bios_vars.cursorY),HL
        EX    DE,HL
        LD    DE,0FFF6H   ; 65526
        LD    B,20H         ; 32 ' '
        INC   HL
        CALL  SUB51
        LD    A,L
        ADD   A,3AH         ; 58 ':'
        LD    C,A
        CALL  bios_printChar
        CALL  SUB57
        POP   HL
        LD    (bios_vars.cursorY),HL
        JP    keyInput

;----------------------------------------------------------------------

        ; Продолжение разбора кода нажатой клавиши
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
        KEY_HANDLER  09h, keyHandler_Tab    ; Tab       - курсор на 4 (8) позиций влево, с привязкой по сетке (?)
        KEY_HANDLER  1Fh, keyHandler_CTP    ; СТР       - выход без сохранения!
        KEY_HANDLER  02h, keyHandler_F3_2   ; F3 снова?
        KEY_HANDLER  06h, keyHandler_F7     ; F7        - поиск текста в строке курсора и ниже (перематывает документ на строку с текстом)
        KEY_HANDLER  0Ch, keyHandler_Home   ; Home      - курсор в начало строки
        KEY_HANDLER  0Ah, keyHandler_End    ; End (ПС)  - курсор в конец строки
        KEY_HANDLER  1Bh, keyHandler_Esc    ; Esc       - режим Esc-последовательности
        DB  00H

;----------------------------------------------------------------------

        ; Обработка нажатия Esc
keyHandler_Esc:
        CALL    SUB15

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
        KEY_HANDLER  56h, keyHandler_Esc_V      ; Esc-V    - загрузить файл с магнитофона (в другом формате?)
        KEY_HANDLER  47h, keyHandler_Esc_G      ; Esc-G    - загрузить файл с магнитофона и вставить в конец документа
        KEY_HANDLER  4Eh, keyHandler_Esc_N      ; Esc-N    - новый файл (очистить буфер)
        KEY_HANDLER  4Ah, keyHandler_Esc_J      ; Esc-J    - объединить текущую строку и следующую
        KEY_HANDLER  53h, keyHandler_Esc_S      ; Esc-S    - разбить строку в положении курсора
        KEY_HANDLER  43h, keyHandler_Esc_C      ; Esc-C    - вставить выделенные строки ниже текущей
        KEY_HANDLER  4Dh, keyHandler_Esc_M      ; Esc-M    - переместить выделенные строки ниже текущей
        DB  00H

;----------------------------------------------------------------------

keyHandler_CTP:
        COLOR COLOR_BIOS
        LD    C,1FH
        CALL  bios_printChar
        CALL  SUB15
        CALL  SUB54
        JP    bios_reboot

keyHandler_F3_2:
        CALL  SUB15
        COLOR COLOR_BIOS
        LD    C,1FH
        CALL  bios_printChar
        CALL  SUB54
        RET

keyHandler_Esc_Home:
        LD    HL,0001H    ; 1; [4]
        LD    (VAR8),HL
        LD    HL,(VAR01)
        LD    (VAR1),HL
        LD    (VAR3),HL
SUB1:   LD    HL,(VAR1)   ; [10]
        EX    DE,HL
        LD    HL,(VAR3)
        CALL  SUB38
        JP    Z,SUB2
        CALL  SUB49
        JP    SUB1
SUB2:   CALL  SUB55         ; [2]
SUB3:   LD    BC,180CH    ; 6156; [2]
LBL14:  CALL  bios_printChar
        CALL  SUB7
        INC   A
        JP    Z,SUB4
        DEC   B
        LD    C,0AH         ; 10
        JP    NZ,LBL14
SUB4:   CALL  SUB53         ; [2]
        LD    C,0CH         ; 12
        JP    bios_printChar
LBL15:  CALL  SUB15         ; [3]
SUB5:   CALL  SUB43         ; [5]
        LD    HL,(VAR1)
        CALL  SUB2
        JP    SUB44

keyHandler_Down:
        CALL  SUB15
        CALL  SUB48
        RET   Z
        CALL  SUB50
        LD    A,(bios_vars.cursorY)
        CP    0EEH          ; 238
        JP    NZ,bios_printChar
        LD    HL,(VAR1)
        CALL  SUB42
        LD    (VAR1),HL
        CALL  SUB43
        CALL  SUB45
        LD    HL,(VAR3)
        CALL  SUB7
        JP    SUB44

keyHandler_Up
        CALL  SUB15
        CALL  SUB49
        RET   Z
        CALL  SUB50
        LD    A,(bios_vars.cursorY)
        CP    08H           ; 8
        JP    NZ,bios_printChar
        LD    HL,(VAR3)
        LD    (VAR1),HL
        PUSH  HL
        CALL  SUB46
        POP   HL
        CALL  SUB43
        CALL  SUB7
        JP    SUB44
SUB7:   CALL  keyHandler_Home          ; [4]
        CALL  SUB8
        JP    NC,LBL16
        LD    A,0FFH                ; 255
        LD    (bios_vars.inverse),A
LBL16:  LD    A,(HL)
        CP    0DH           ; 13
        LD    C,20H         ; 32 ' '
        CALL  Z,bios_printChar
LBL17:  LD    A,(HL)
        LD    C,A
        INC   A
        JP    Z,LBL18
        INC   HL
        CALL  bios_printChar
        CP    0EH           ; 14
        JP    NZ,LBL17
LBL18:  XOR   A
        LD    (bios_vars.inverse),A
        LD    (bios_vars.inverse+1),A
        LD    A,C
        RET
SUB8: PUSH  DE            ; [7]
        EX    DE,HL
        LD    HL,(VAR6)
        LD    A,H
        OR    L
        JP    Z,LBL19
        DEC   HL
        CALL  SUB38
        JP    NC,LBL19
        LD    HL,(VAR7)
        DEC   HL
        CALL  SUB38
        CCF
LBL19:  EX    DE,HL         ; [2]
        POP   DE
        RET

keyHandler_Right:
        CALL  SUB36
        JP    Z,LBL117
LBL20:  LD    B,0BAH                ; 186
        JP    LBL21

keyHandler_Left:
        CALL  SUB36
        JP    Z,LBL120
        LD    B,00H
LBL21:  LD    A,(bios_vars.cursorY+1)
        CP    B
        RET   Z
        JP    bios_printChar

keyHandler_End:
        CALL  SUB12
        CALL  SUB10
        LD    HL,LBL2             ; 53258
        CALL  SUB39
        LD    A,E
        JP    LBL48

keyHandler_Home:
        XOR   A             ; [4]
        LD    (bios_vars.cursorY+1),A
        RET

keyHandler_Enter:
        LD    HL,(VAR3)
        CALL  SUB8
        JP    C,Beep4
        CALL  SUB12
        CALL  SUB10
        LD    A,0DH         ; 13
        LD    (DE),A
        CALL  SUB15
        CALL  keyHandler_Home
        LD    HL,(VAR3)
        LD    C,18H         ; 24
LBL23:  LD    A,(HL)
        CP    20H           ; 32 ' '
        JP    NZ,LBL24
        CALL  bios_printChar
        INC   HL
        JP    LBL23
LBL24:  LD    C,1AH         ; 26
        CALL  keyHandler_Down
        JP    SUB46

        ; В REF6 изначально текст "KSOFT '92"
SUB10:  LD    DE, REF6

        ; Пропус пробелов в строке по адресу de
SkipSpaces:
        DEC   DE
        LD    A,(DE)
        CP    20H
        JP    Z,SkipSpaces
        INC   DE
        RET

SUB12:  LD    A,(VAR4)    ; [7]
        OR    A
        RET   NZ
        cpl
        LD    (VAR4),A
        LD    HL,LBL2             ; 53258
        LD    A,40H         ; 64 '@'
LBL25:  LD    (HL),20H    ; 32 ' '
        INC   HL
        DEC   A
        JP    NZ,LBL25
        LD    DE,LBL2             ; 53258
        LD    HL,(VAR3)
        LD    B,40H         ; 64 '@'
LBL26:  LD    A,(HL)
        CP    0DH           ; 13
        RET   Z
        CP    0FFH          ; 255
        RET   Z
        DEC   B
        RET   Z
        LD    (DE),A
        INC   HL
        INC   DE
        JP    LBL26

keyHandler_Space:
        LD    HL,(VAR3)
        CALL  SUB8
        JP    C,Beep4
        CALL  SUB12
        CALL  SUB13
        LD    A,(VAR5)
        OR    A
        JP    NZ,LBL28
        LD    (HL),C
        CALL  bios_printChar
        LD    A,(bios_vars.cursorY+1)
        CP    0BDH          ; 189
        RET   NZ
        LD    C,08H         ; 8
        JP    bios_printChar
LBL28:  LD    DE,REF5             ; 53321
        CALL  SkipSpaces
        PUSH  HL
        LD    HL,REF5             ; 53321
        CALL  SUB38
        JP    Z,LBL29
        INC   DE
LBL29:  POP   HL
        CALL  SUB43
LBL30:  LD    B,(HL)
        LD    (HL),C
        CALL  bios_printChar
        LD    C,B
        INC   HL
        CALL  SUB38
        JP    C,LBL30
        CALL  SUB44
        LD    C,18H         ; 24
        JP    LBL20

keyHandler_F1:
        LD    HL,VAR5             ; 53340
        LD    A,(HL)
        cpl
        LD    (HL),A
        JP    SUB47
SUB13:  LD    HL,LBL2             ; 53258; [4]
SUB14:  LD    A,(bios_vars.cursorY+1)        ; [4]
        OR    A
LBL32:  RET   Z
        INC   HL
        SUB   03H           ; 3
        JP    LBL32

keyHandler_Del:
        CALL  SUB12
        CALL  SUB13
        PUSH  HL
        LD    C,L
        LD    B,H
        INC   BC
        LD    DE,REF5             ; 53321
        CALL  SUB37
        POP   HL
        LD    DE,REF5             ; 53321
        CALL  SkipSpaces
        PUSH  HL
        LD    HL,REF5             ; 53321
        CALL  SUB38
        JP    Z,LBL34
        INC   DE
LBL34:  POP   HL
        CALL  SUB43
LBL35:  LD    C,(HL)
        CALL  bios_printChar
        CALL  SUB38
        INC   HL
        JP    C,LBL35
        JP    SUB44

SUB15:  LD    A,(VAR4)  ; читаем VAR4
        OR    A
        RET   Z         ; если VAR4 == 0, выходим
        XOR   A         ; инвертируем
        LD    (VAR4),A  ; сохраняем обртано
        PUSH  BC
        CALL  SUB10
        LD    HL,LBL2
        CALL  SUB39
        LD    HL,(VAR3)
        EX    DE,HL
        ADD   HL,DE
        EX    DE,HL
        CALL  SUB42
        INC   A
        JP    Z,LBL36
        DEC   HL
LBL36:  CALL  SUB16
        LD    HL,(VAR3)
        LD    BC,LBL2             ; 53258
        CALL  SUB38
        CALL  NZ,SUB37
        POP   BC
        RET
SUB16:  CALL  SUB38         ; [5]
        RET   Z
        PUSH  AF
        PUSH  DE
        CALL  SUB39
        LD    B,D
        LD    C,E
        EX    DE,HL
        LD    HL,(VAR6)
        CALL  SUB38
        JP    C,LBL37
        ADD   HL,BC
        LD    (VAR6),HL
LBL37:  LD    HL,(VAR7)
        EX    DE,HL
        CALL  SUB38
        EX    DE,HL
        JP    NC,LBL38
        ADD   HL,BC
        LD    (VAR7),HL
LBL38:  EX    DE,HL
        POP   DE
        POP   AF
        JP    C,LBL42
        PUSH  DE
        LD    C,E
        LD    B,D
        EX    DE,HL
        LD    HL,(VAR2)
        INC   HL
        EX    DE,HL
        CALL  SUB39
LBL39:  LD    A,E
        AND   0FEH          ; 254
        OR    D
        JP    Z,LBL40
        LD    A,(HL)
        LD    (BC),A
        INC   BC
        INC   HL
        DEC   DE
        LD    A,(HL)
        LD    (BC),A
        INC   BC
        INC   HL
        DEC   DE
        JP    LBL39
LBL40:  CP    E
        JP    Z,LBL41
        LD    A,(HL)
        LD    (BC),A
        INC   BC
LBL41:  DEC   BC
        LD    L,C
        LD    H,B
        LD    (VAR2),HL
        POP   DE
        RET
LBL42:  PUSH  DE
        CALL  SUB39
        LD    HL,(VAR2)
        LD    B,H
        LD    C,L
        ADD   HL,DE
        POP   DE
        PUSH  DE
        PUSH  HL
        EX    DE,HL
        LD    HL,(VAR03)
        CALL  SUB38
        JP    C,LBL71
        POP   HL
        POP   DE
        LD    (VAR2),HL
        PUSH  HL
        EX    DE,HL
        CALL  SUB39
        EX    (SP),HL
        INC   DE
LBL43:  LD    A,E
        AND   0FCH          ; 252
        OR    D
        JP    Z,LBL44
        LD    A,(BC)
        LD    (HL),A
        DEC   HL
        DEC   BC
        DEC   DE
        LD    A,(BC)
        LD    (HL),A
        DEC   HL
        DEC   BC
        DEC   DE
        LD    A,(BC)
        LD    (HL),A
        DEC   HL
        DEC   BC
        DEC   DE
        LD    A,(BC)
        LD    (HL),A
        DEC   HL
        DEC   BC
        DEC   DE
        JP    LBL43
LBL44:  LD    A,E
        OR    A
        JP    Z,LBL46
LBL45:  LD    A,(BC)
        LD    (HL),A
        DEC   HL
        DEC   BC
        DEC   E
        JP    NZ,LBL45
LBL46:  POP   DE
        RET

keyHandler_Tab:
        CALL  SUB36
        JP    Z,LBL51
        LD    HL,0001H    ; 1
        CALL  SUB14
        LD    E,L
        LD    HL,REF7             ; 53347
        LD    C,03H         ; 3
        XOR   A
LBL47:  DEC   C
        JP    Z,LBL50
        ADD   A,(HL)
        CP    E
        INC   HL
        JP    C,LBL47
LBL48:  CP    3EH           ; 62 '>'; [2]
        JP    C,LBL49
        LD    A,3EH         ; 62 '>'
LBL49:  LD    L,A           ; [3]
        ADD   A,A
        ADD   A,L
        LD    (bios_vars.cursorY+1),A
        RET
LBL50:  ADD   A,(HL)                ; [2]
        CP    E
        JP    C,LBL50
        JP    LBL48
LBL51:  LD    HL,0000H
        CALL  SUB14
        LD    E,L
        LD    HL,REF7             ; 53347
        LD    C,03H         ; 3
        XOR   A
LBL52:  DEC   C
        JP    Z,LBL53
        ADD   A,(HL)
        CP    E
        INC   HL
        JP    C,LBL52
        DEC   HL
        SUB   (HL)
        JP    LBL49
LBL53:  ADD   A,(HL)                ; [2]
        CP    E
        JP    C,LBL53
        SUB   (HL)
        JP    LBL49

keyHandler_F2:
        CALL  SUB15         ; [2]
        LD    HL,(VAR01)
        EX    DE,HL
        LD    HL,(VAR1)
        LD    B,16H         ; 22
LBL55:  CALL  SUB38
        JP    Z,LBL56
        CALL  SUB35
        PUSH  HL
        CALL  SUB49
        POP   HL
        DEC   B
        JP    NZ,LBL55
LBL56:  LD    (VAR1),HL
        CALL  SUB5
        RET

keyHandler_F3:
        CALL  SUB15
        LD    HL,(VAR1)
        LD    BC,1619H    ; 5657
LBL58:  LD    E,L
        LD    D,H
        CALL  SUB42
        LD    A,(HL)
        INC   A
        JP    NZ,LBL59
        LD    L,E
        LD    H,D
        JP    LBL60
LBL59:  PUSH  HL
        CALL  SUB48
        CALL  Z,bios_printChar
        POP   HL
        DEC   B
        JP    NZ,LBL58
LBL60:  LD    (VAR1),HL
        CALL  SUB5
        RET

keyHandler_Esc_End:
        CALL  SUB48
        JP    NZ,keyHandler_Esc_End
        LD    HL,(VAR3)
        LD    (VAR1),HL
        LD    A,08H         ; 8
        LD    (bios_vars.cursorY),A
        JP    keyHandler_F2

keyHandler_Esc_L:
        LD    HL,REF23    ; 54885
        PUSH  HL
        LD    HL,(VAR6)
        LD    A,H
        OR    L
        JP    NZ,LBL62
        CALL  SUB17
        JP    LBL63

LBL62:  EX    DE,HL
        LD    HL,(VAR3)
        CALL  SUB42
        EX    DE,HL
        CALL  SUB38
        JP    C,LBL63
SUB17:  LD    HL,(VAR3)
        LD    (VAR6),HL
        RET

LBL63:  LD    HL,(VAR3)   ; [2]
        CALL  SUB42
        LD    (VAR7),HL
        RET

keyHandler_Esc_D:
        LD    HL,(VAR6)
        LD    A,L
        OR    H
        JP    Z,Beep4
        EX    DE,HL
        LD    HL,(VAR3)
LBL64:  CALL  SUB8
        JP    NC,LBL65
        CALL  SUB35
        JP    NZ,LBL64
LBL65:  LD    (VAR3),HL
        LD    HL,0000H
        LD    (VAR6),HL
        LD    HL,(VAR7)
LBL66:  PUSH  DE
        CALL  SUB39
        LD    B,D
        LD    C,E
        EX    DE,HL
        LD    HL,(VAR1)
        CALL  SUB38
        JP    C,LBL67
        ADD   HL,BC
        LD    (VAR1),HL
LBL67:  LD    HL,(VAR3)
        CALL  SUB38
        JP    C,LBL68
        ADD   HL,BC
        LD    (VAR3),HL
LBL68:  EX    DE,HL
        POP   DE
        CALL  SUB16
        CALL  SUB52
        LD    A,(bios_vars.cursorY)
        SUB   08H           ; 8
        LD    B,A
        JP    Z,LBL70
LBL69:  CALL  SUB35
        CALL  Z,SUB18
        LD    A,B
        SUB   0AH           ; 10
        LD    B,A
        JP    NZ,LBL69
LBL70:  LD    (VAR1),HL
        JP    SUB5
SUB18:  PUSH  HL
        CALL  SUB48
        POP   HL
        RET   NZ
        LD    C,19H         ; 25
        JP    bios_printChar

keyHandler_Esc_U:
        LD    HL,(VAR6)
        LD    DE,0000H
        CALL  SUB38
        RET   Z
        EX    DE,HL
        LD    (VAR6),HL
REF23:  CALL  SUB43
        LD    HL,(VAR1)
        CALL  SUB3
        JP    SUB44
LBL71:  LD    HL,REF8             ; 53351; [2]
        CALL  bios_printString
        CALL  Beep4
        CALL  bios_getch
        JP    LBL5

keyHandler_Esc_O:
        LD    HL,REF9             ; 53370
        CALL  bios_printString
        CALL  SUB33
        JP    NZ,SUB1
        LD    HL,(VAR01)
        CALL  SUB32
        PUSH  BC
        PUSH  DE
        CALL  SUB19
        EX    (SP),HL
        EX    DE,HL
        LD    HL,LBL2
        LD    A,0E6H
        LD    B,05H
LBL72:  CALL  SUB24
        DEC   B
        JP    NZ,LBL72
LBL73:  LD    A,(HL)
        CALL  SUB24
        OR    A
        JP    Z,LBL74
        INC   HL
        JP    LBL73
LBL74:  CALL  SUB20
        POP   DE
        LD    A,(VAR01)
        cpl
        INC   A
        LD    L,A
        LD    A,(VAR02)
        cpl
        INC   A
        LD    H,A
        ADD   HL,DE
        LD    A,0E6H
        CALL  SUB24
        LD    A,L
        cpl
        CALL  SUB24
        LD    A,H
        cpl
        LD    HL,(VAR01)
        CALL  SUB23
        POP   BC
        LD    A,C
        CALL  SUB24
        LD    A,B
        CALL  SUB24
        JP    SUB1
SUB19:  LD    D,04H         ; 4
        XOR   A
LBL75:  LD    E,40H         ; 64 '@'
        XOR   55H           ; 85 'U'
        CALL  SUB22
        DEC   D
        JP    NZ,LBL75
        RET
SUB20:  CALL  SUB21
SUB21:  XOR   A
        LD    E,A
SUB22:  CALL  SUB24         ; [2]
        DEC   E
        JP    NZ,SUB22
        RET
SUB23:  CALL  SUB24         ; [2]
        CALL  SUB38
        LD    A,(HL)
        INC   HL
        JP    NZ,SUB23
        JP    SUB24
SUB24:  LD    C,A           ; [9]
        JP    bios_tapeWrite

keyHandler_Esc_I:
        LD    B,00H

LBL76:  LD    A,B           ; [2]
        LD    (VAR9),A
        LD    HL,0000H
        LD    (VAR6),HL
        LD    HL,REF9             ; 53370
        CALL  bios_printString
        CALL  SUB33
        JP    NZ,SUB1
        CALL  SUB31
        PUSH  HL
LBL77:  CALL  SUB30
        LD    B,A
        LD    A,(VAR9)
        INC   A
        JP    NZ,LBL78
        LD    A,B
        CP    (HL)
        JP    NZ,LBL79
LBL78:  LD    (HL),B
        INC   HL
        INC   B
        JP    NZ,LBL77
        LD    A,08H
        CALL  SUB28
        POP   HL
        PUSH  BC
        CALL  SUB32
        EX    (SP),HL
        LD    D,B
        LD    E,C
        CALL  SUB38
        JP    NZ,LBL79
        POP   HL
        LD    (VAR2),HL
        JP    keyHandler_Esc_Home

LBL79:  POP   HL
        LD    HL,REF12
        CALL  bios_printString
        CALL  Beep4
        CALL  bios_getch
        LD    HL,(VAR2)
        LD    (HL),0FFH
        JP    keyHandler_Esc_Home

SUB25:  LD    HL,SMC6+1 ; тут адрес перехода в п/п LBL3
        CALL  SUB29
LBL80:  CALL  SUB30
        LD    (HL),A
        OR    A
        JP    Z,LBL81
        INC   HL
        JP    LBL80

LBL81:  LD    HL,REF10
        CALL  bios_printString
        LD    HL,SMC6+1 ; тут адрес перехода в п/п LBL3
        PUSH  HL
        CALL  bios_printString
        CALL  SUB26
        POP   HL
        LD    DE,LBL2
LBL82:  LD    A,(DE)
        OR    A
        RET   Z
        CP    (HL)
        INC   HL
        INC   DE
        JP    Z,LBL82
        RET
SUB26:  LD    HL,0000H
        LD    E,L
        LD    D,H
LBL83:  DEC   HL
        CALL  SUB38
        RET   Z
        JP    LBL83
SUB27:  LD    A,0FFH
SUB28:  CALL  bios_tapeRead
        LD    C,A
        CALL  SUB30
        LD    B,A
        RET
LBL84:  XOR   A
        LD    (bios_vars.tapeInverse),A

SUB29:  LD    B,04H
        LD    A,0FFH
LBL85:  CALL  bios_tapeRead
        CP    0E6H
        JP    NZ,LBL84
        DEC   B
        LD    A,08H
        JP    NZ,LBL85
        RET

SUB30:  LD    A,08H
        JP    bios_tapeRead

SUB31:  CALL  SUB25
        JP    NZ,SUB31
        CALL  SUB27
        LD    HL,(VAR01)
        LD    A,(VAR9)
        DEC   A
        JP    M,LBL86
        LD    HL,(VAR2)
LBL86:  LD    A,B
        cpl
        LD    B,A
        LD    A,C
        cpl
        LD    C,A
        PUSH  HL
        EX    DE,HL
        LD    HL,(VAR03)
        EX    DE,HL
        ADD   HL,BC
        CALL  SUB38
        POP   HL
        RET   C
        JP    LBL71

keyHandler_Esc_V:
        LD    B,0FFH                ; 255
        JP    LBL76

keyHandler_Esc_G:
        LD    B,01H         ; 1
        JP    LBL76

SUB32:  LD    BC,0000H    ; [2]
LBL87:  LD    A,(HL)
        CP    0FFH          ; 255
        RET   Z
        ADD   A,C
        LD    C,A
        LD    A,00H
        ADC   A,B
        LD    B,A
        INC   HL
        JP    LBL87
SUB33:  LD    B,30H         ; 48 '0'; [2]
        LD    HL,LBL2             ; 53258
        LD    E,L
        LD    D,H
LBL88:  CALL  bios_getch          ; [5]
SUB34:  CP    20H           ; 32 ' '
        JP    C,LBL90
        DEC   B
        JP    NZ,LBL89
        INC   B
        JP    LBL88
LBL89:  LD    C,A
        CALL  bios_printChar
        LD    (HL),A
        INC   HL
        JP    LBL88
        LD    BC,2A20H    ; 10784
        CALL  keyHandler_Home
        JP    TypeCharCtimesB
LBL90:  CP    1FH           ; 31
        JP    NZ,LBL91
        OR    A
        RET
LBL91:  CP    08H           ; 8
        JP    NZ,LBL92
        CALL  SUB38
        JP    Z,LBL88
        PUSH  HL
        LD    HL,REF11    ; 53380
        CALL  bios_printString
        POP   HL
        DEC   HL
        INC   B
        JP    LBL88
LBL92:  CP    0DH           ; 13
        JP    NZ,LBL88
        XOR   A
        LD    (HL),A
        RET

keyHandler_F7:
        CALL  SUB15
        LD    HL,06F9H    ; 1785
        LD    (bios_vars.cursorY),HL
        CALL  SUB56
        LD    HL,REF13    ; 53395
        CALL  bios_printString
        LD    HL,1EF9H    ; 7929
        LD    (bios_vars.cursorY),HL
        CALL  bios_getch
        CP    0DH           ; 13
        JP    Z,LBL93
        LD    BC,1020H    ; 4128
        CALL  TypeCharCtimesB
        LD    (bios_vars.cursorY),HL
        LD    HL,REF14    ; 53403
        LD    E,L
        LD    D,H
        LD    B,0FH         ; 15
        CALL  SUB34
        PUSH  AF
        CALL  SUB57
        POP   AF
        JP    NZ,SUB1
LBL93:  CALL  SUB57
        LD    HL,(VAR1)
        CALL  SUB42
LBL94:  LD    DE,REF14    ; 53403
LBL95:  LD    B,(HL)
        LD    A,(DE)
        OR    A
        JP    Z,LBL96
        CP    B
        INC   HL
        INC   DE
        JP    Z,LBL95
        INC   B
        JP    NZ,LBL94
        CALL  Beep4
        JP    keyHandler_Esc_Home
LBL96:  CALL  SUB35
        LD    (VAR1),HL
        LD    (VAR3),HL
        CALL  SUB52
        JP    SUB1

keyHandler_Esc_J:
        CALL  SUB12
        CALL  SUB10
        LD    HL,(VAR3)
        CALL  SUB42
        LD    A,(HL)
        INC   A
        RET   Z
        DEC   HL
        LD    (HL),20H    ; 32 ' '
        INC   HL
        LD    C,L
        LD    B,H
        LD    HL,REF3             ; 53319
LBL97:  CALL  SUB38
        JP    C,LBL15
        LD    A,(BC)
        CP    0DH           ; 13
        JP    Z,LBL15
        CP    0FFH          ; 255
        JP    Z,LBL15
        LD    (DE),A
        INC   DE
        INC   BC
        JP    LBL97

keyHandler_Esc_S:
        LD    HL,(VAR3)
        CALL  SUB14
        LD    E,L
        LD    D,H
        INC   DE
        PUSH  HL
        CALL  SUB16
        POP   HL
        LD    (HL),0DH    ; 13
        JP    SUB5

SUB35:  PUSH  DE            ; [6]
        EX    DE,HL
        LD    HL,(VAR01)
        EX    DE,HL
        CALL  SUB38
        JP    Z,LBL101
        DEC   HL
        CALL  SUB38
        JP    Z,LBL100
        DEC   HL
LBL98:  LD    A,(HL)
        CP    0DH           ; 13
        JP    Z,LBL99
        CALL  SUB38
        DEC   HL
        JP    NZ,LBL98

LBL99:  INC   HL
LBL100: XOR   A
        INC   A

LBL101: POP   DE
        RET

keyHandler_Esc_C:
        LD    HL,(VAR7)
        EX    DE,HL
        LD    HL,(VAR6)
        LD    A,H
        OR    L
        JP    Z,Beep4
        CALL  SUB39
        LD    HL,(VAR3)
        CALL  SUB8
        PUSH  AF
        CALL  SUB42
        POP   AF
        CALL  C,SUB8
        JP    C,Beep4
        PUSH  HL
        EX    DE,HL
        ADD   HL,DE
        EX    DE,HL
        CALL  SUB16
        LD    HL,(VAR6)
        LD    C,L
        LD    B,H
        POP   HL
        CALL  SUB37
        JP    SUB5

keyHandler_Esc_M:
        LD    HL,(VAR7)
        EX    DE,HL
        LD    HL,(VAR6)
        LD    A,H
        OR    L
        JP    Z,Beep4
        CALL  SUB39
        LD    HL,(VAR3)
        CALL  SUB8
        JP    C,Beep4
        CALL  SUB42
        PUSH  HL
        EX    DE,HL
        ADD   HL,DE
        EX    DE,HL
        POP   BC
        PUSH  DE
        PUSH  HL
        PUSH  BC
        CALL  SUB16
        LD    HL,(VAR6)
        LD    C,L
        LD    B,H
        POP   HL
        CALL  SUB37
        LD    HL,(VAR6)
        EX    DE,HL
        POP   HL
        LD    (VAR6),HL
        LD    HL,(VAR7)
        EX    (SP),HL
        LD    (VAR7),HL
        POP   HL
        JP    LBL66

keyHandler_Esc_N:
        LD    HL,txtNew    ; 53419
        CALL  bios_printString
        CALL  bios_getch
        CP    59H           ; 89 'Y'
        JP    NZ,SUB1
        LD    HL,(VAR01)
        LD    (HL),0DH    ; 13
        INC   HL
        LD    (HL),0FFH   ; 255
        LD    (VAR2),HL
        JP    keyHandler_Esc_Home

keyHandler_F8:
        XOR   A
        LD    (VAR4),A
        CALL  SUB43
        CALL  keyHandler_Home
        LD    BC,3F20H    ; 16160
        CALL  TypeCharCtimesB
        LD    HL,(VAR3)
        CALL  SUB7
        JP    SUB44
SUB36:  LD    A,(IO_KEYB_B) ; [3]
        AND   02H           ; 2
        RET
SUB37:  LD    A,(BC)        ; [5]
        LD    (HL),A
        INC   HL
        INC   BC
        CALL  SUB38
        JP    NZ,SUB37
        RET
SUB38:  LD    A,H           ; [34]
        CP    D
        RET   NZ
        LD    A,L
        CP    E
        RET
SUB39:  LD    A,E           ; [9]
        SUB   L
        LD    E,A
        LD    A,D
        SBC   A,H
        LD    D,A
        RET

        ; Звуковой сигнал 4 раза через п/п печати символа 07h
Beep4:
        LD    BC,0407H      ; b = счетчик цикла, c = код символа

        ; Печать b раз символа с кодом c
TypeCharCtimesB:
        CALL  bios_printChar
        DEC   B
        JP    NZ,TypeCharCtimesB
        RET

SUB42:  LD    A,(HL)                ; [11]
        CP    0FFH          ; 255
        RET   Z
        CP    0DH           ; 13
        INC   HL
        JP    NZ,SUB42
        RET
SUB43:  PUSH  HL            ; [7]
        LD    HL,(bios_vars.cursorY)
        LD    (SMC2+1),HL
        POP   HL
        RET
SUB44:  PUSH  HL            ; [7]
SMC2:   LD    HL,0000H
        LD    (bios_vars.cursorY),HL
        POP   HL
        RET
LBL103: CALL  SUB54
        CALL  SUB35
        LD    (VAR3),HL
        LD    (VAR1),HL
        CALL  SUB52
        CALL  SUB53
        CALL  SUB3
        CALL  bios_getch
        PUSH  AF
        LD    HL,06F9H    ; 1785
        LD    (bios_vars.cursorY),HL
        LD    BC,3020H    ; 12320
        CALL  TypeCharCtimesB
        CALL  SUB4
        POP   AF
        LD    HL,REF21    ; 53535
        PUSH  HL
        JP    keyInput2
SUB45:  LD    H,90H         ; 144
        LD    D,H
LBL104: LD    C,39H         ; 57 '9'
        LD    L,01H         ; 1
        LD    E,0BH         ; 11
LBL105: LD    A,(DE)
        LD    (HL),A
        INC   HL
        INC   DE
        LD    A,(DE)
        LD    (HL),A
        INC   HL
        INC   DE
        LD    A,(DE)
        LD    (HL),A
        INC   HL
        INC   DE
        LD    A,(DE)
        LD    (HL),A
        INC   HL
        INC   DE
        DEC   C
        JP    NZ,LBL105
        LD    C,0AH         ; 10
        XOR   A
LBL106: LD    (HL),A
        INC   HL
        DEC   C
        JP    NZ,LBL106
        LD    A,H
        CP    0BFH          ; 191
        RET   Z
        INC   H
        INC   D
        JP    LBL104
SUB46:  LD    A,(bios_vars.cursorY)  ; [2]
        LD    L,A
        LD    A,0EEH                ; 238
        SUB   L
        OR    A
        rra
        LD    (SMC3+1),A
        LD    H,0BFH                ; 191
        LD    D,H
        LD    B,30H         ; 48 '0'
LBL107: LD    L,0EFH                ; 239
        LD    E,0E5H                ; 229
SMC3:   LD    A,00H
        LD    C,A
        OR    A
        JP    Z,LBL109
LBL108: LD    A,(DE)
        LD    (HL),A
        DEC   HL
        DEC   DE
        LD    A,(DE)
        LD    (HL),A
        DEC   HL
        DEC   DE
        DEC   C
        JP    NZ,LBL108
LBL109: LD    C,09H         ; 9
        XOR   A
LBL110: LD    (HL),A
        DEC   HL
        DEC   DE
        DEC   C
        JP    NZ,LBL110
        DEC   H
        LD    D,H
        DEC   B
        JP    NZ,LBL107
        RET

        ; Обновление стаус бара
SUB47:  PUSH  HL
        LD    HL,(bios_vars.cursorY)
        PUSH  HL
        CALL  SUB56
        LD    HL,9DF9H    ; 40441
        LD    (bios_vars.cursorY),HL
        LD    A,(VAR5)
        OR    A
        LD    HL,txtIns
        JP    NZ,LBL111
        LD    HL,txtOvr
LBL111: CALL  bios_printString
        CALL  SUB57
        POP   HL
        LD    (bios_vars.cursorY),HL
        POP   HL
        RET

SUB48:  LD    HL,(VAR3)   ; [5]
        CALL  SUB42
        LD    A,(HL)
        INC   A
        RET   Z
        LD    (VAR3),HL
        LD    HL,(VAR8)
        INC   HL
        LD    (VAR8),HL
        XOR   A
        INC   A
        RET
SUB49:  LD    HL,(VAR3)   ; [3]
        CALL  SUB35
        RET   Z
        LD    (VAR3),HL
        LD    HL,(VAR8)
        DEC   HL
        LD    (VAR8),HL
        RET
SUB50:  PUSH  DE            ; [3]
        PUSH  BC
        PUSH  HL
        LD    HL,(bios_vars.cursorY)
        PUSH  HL
        CALL  SUB56
        LD    HL,67F9H    ; 26617
        LD    (bios_vars.cursorY),HL
        LD    HL,(VAR8)
        LD    B,20H         ; 32 ' '
        LD    DE,0FC18H   ; 64536
        CALL  SUB51
        LD    DE,03E8H    ; 1000
        ADD   HL,DE
        LD    DE,0FF9CH   ; 65436
        CALL  SUB51
        LD    DE,0064H    ; 100
        ADD   HL,DE
        LD    DE,0FFF6H   ; 65526
        CALL  SUB51
        LD    A,L
        ADD   A,3AH         ; 58 ':'
        LD    C,A
        CALL  bios_printChar
        CALL  SUB57
        POP   HL
        LD    (bios_vars.cursorY),HL
        POP   HL
        POP   BC
        POP   DE
        RET
SUB51:  LD    C,2FH         ; 47 '/'; [4]
LBL112: INC   C
        ADD   HL,DE
        JP    C,LBL112
        LD    A,C
        CP    30H           ; 48 '0'
        JP    Z,LBL113
        LD    B,0FFH                ; 255
LBL113: AND   B
        LD    C,A
        JP    bios_printChar
SUB52:  LD    HL,(VAR3)   ; [3]
        EX    DE,HL
        LD    HL,(VAR01)
        LD    (VAR3),HL
        LD    HL,0001H    ; 1
        LD    (VAR8),HL
LBL114: LD    HL,(VAR3)
        CALL  SUB38
        RET   Z
        CALL  SUB48
        JP    LBL114
SUB53:  PUSH  HL            ; [2]
        LD    HL,(bios_vars.cursorY)
        PUSH  HL
        CALL  SUB56
        LD    H,90H
        LD    C,30H
LBL115: LD    L,0F1H
        LD    A,09H
LBL116: LD    (HL),0FFH
        INC   L
        DEC   A
        JP    NZ,LBL116
        INC   H
        DEC   C
        JP    NZ,LBL115
        LD    HL,58F9H
        LD    (bios_vars.cursorY),HL
        LD    HL,txtLine
        CALL  bios_printString
        LD    HL,80F9H
        LD    (bios_vars.cursorY),HL
        LD    HL,txtCol
        CALL  bios_printString
        CALL  SUB50
        CALL  SUB47
        CALL  SUB57
        POP   HL
        LD    (bios_vars.cursorY),HL
        POP   HL
        RET

SUB54:  POP   HL

        ; Установка указателя стека, агрумент этой инструкции
        ; может меняться из п/п LBL4
SMC4:   LD    SP,8F50H
        ; Переход по адресу в hl
        JP    (HL)

LBL117: CALL  SUB12
        CALL  SUB13
        LD    DE,REF4             ; 53320
        LD    C,18H         ; 24
LBL118: CALL  SUB38
        RET   Z
        LD    A,(HL)
        CP    20H           ; 32 ' '
        JP    Z,LBL119
        INC   HL
        CALL  bios_printChar
        JP    LBL118
LBL119: CALL  SUB38         ; [2]
        JP    Z,keyHandler_End
        LD    A,(HL)
        CP    20H           ; 32 ' '
        RET   NZ
        INC   HL
        CALL  bios_printChar
        JP    LBL119
LBL120: CALL  SUB12
        CALL  SUB13
        LD    DE,LBL2             ; 53258
        LD    C,08H         ; 8
        CALL  SUB38
        RET   Z
        DEC   HL
        DEC   DE
LBL121: CALL  SUB38
        RET   Z
        LD    A,(HL)
        CP    20H           ; 32 ' '
        JP    NZ,LBL122
        CALL  bios_printChar
        DEC   HL
        JP    LBL121
LBL122: CALL  SUB38         ; [2]
        RET   Z
        LD    A,(HL)
        CP    20H           ; 32 ' '
        RET   Z
        DEC   HL
        CALL  bios_printChar
        JP    LBL122
SUB55:  PUSH  HL
        PUSH  DE
        LD    HL,0000H
        ADD   HL,SP
        LD    (SMC5+1),HL
        LD    DE,0000H
        LD    HL,0BFF0H   ; 49136
        LD    C,30H         ; 48 '0'
LBL123: LD    SP,HL
        LD    A,0FH         ; 15
LBL124: PUSH  DE
        PUSH  DE
        PUSH  DE
        PUSH  DE
        PUSH  DE
        PUSH  DE
        PUSH  DE
        PUSH  DE
        DEC   A
        JP    NZ,LBL124
        DEC   H
        DEC   C
        JP    NZ,LBL123
SMC5:   LD    SP,0FFFFH   ; 65535
        POP   DE
        POP   HL
        RET
SUB56:  PUSH  HL            ; [5]
        LD    HL,0FFFFH   ; 65535
LBL125: LD    (bios_vars.inverse),HL
        POP   HL
        RET
SUB57:  PUSH  HL            ; [6]
        LD    HL,0000H
        JP    LBL125

;----------------------------------------------------------------------
; Запрос имени файла в строке состояния

inputFileName:
        CALL  SUB63
        LD    DE,BUFFER1
        LD    BC,inputBuf
        CALL  bios_memcpy_bc_hl
        LD    HL,00F9H
        LD    (bios_vars.cursorY),HL
        LD    HL,0FFFFH
        LD    (bios_vars.inverse),HL
        LD    HL,txtFile
        CALL  bios_printString
        CALL  bios_getch
        LD    HL,0FF9H
        LD    (bios_vars.cursorY),HL
        CP    0DH           ; 13
        JP    Z,LBL126
        PUSH  AF
        LD    HL,txtSpace12
        CALL  bios_printString
        LD    HL,0FF9H
        LD    (bios_vars.cursorY),HL
        LD    HL,inputBuf
        LD    BC,inputBuf
        LD    DE,inputBufEnd    ; 56781
        POP   AF
        JP    LBL131

LBL126: LD    HL,inputBuf
        LD    DE,inputBufEnd
        LD    BC,BUFFER2
        CALL  bios_memcpy_bc_hl
        LD    HL,0000H
        LD    (bios_vars.inverse),HL
        RET

;----------------------------------------------------------------------
; Загрузка файла с диска

openFile:
        CALL  prepareFileName
        LD    HL,(VAR01)
        EX    DE,HL
        LD    HL,fileDescName
        CALL  bios_fileLoad2
        JP    C,LBL135
        JP    LBL126

;----------------------------------------------------------------------
; Подготовка имени файла для файловых операций

prepareFileName:
        LD    HL,inputBuf
        LD    DE,fileDescName
        CALL  bios_fileNamePrepare
        RET

;----------------------------------------------------------------------
; Сохранение файла на диск

saveFile:
        CALL  prepareFileName
        LD    DE,0000H
        LD    B,00H
        LD    HL,(VAR01)

getCRC: ; Поиск конца буфера и подсчет контрольной суммы
    IF STANDARD_EDITOR
        ; В MXOS 2 контрольная сумма не нужна
        LD    A,B
        ADD   A,(HL)
        LD    B,A
    ENDIF
        INC   DE
        LD    A,(HL)
        INC   A
        JP    Z,LBL128
        INC   HL
        JP    getCRC

        ; Формирование дескриптора файла
LBL128: LD    HL,(VAR01)
        LD    (fileDescAddr),HL
        EX    DE,HL
        LD    (fileDescSize),HL

    IF STANDARD_EDITOR
        ; В MXOS 2 контрольная сумма не нужна
        LD    A,B
        LD    (fileDescCRC),A
    ENDIF

        ; Сохранение файла
        LD    HL,fileDescName
        CALL  bios_fileCreate
        JP    C,LBL135
        JP    LBL126

;----------------------------------------------------------------------

LBL129: POP   AF
        JP    SUB1
        LD    B,H
        LD    C,L
LBL130: CALL  bios_getch          ; [4]
LBL131: CP    08H           ; 8
        JP    Z,LBL132
        CP    0DH           ; 13
        JP    Z,LBL134
        CP    1FH           ; 31
        JP    Z,LBL140
        CP    20H           ; 32 ' '
        JP    C,LBL130
        PUSH  AF
        CALL  bios_cmp_hl_de
        JP    Z,LBL129
        POP   AF
        LD    (HL),A
        PUSH  BC
        LD    C,A
        CALL  bios_printChar
        POP   BC
        INC   HL
        JP    LBL130
LBL132: LD    A,H
        CP    B
        JP    NZ,LBL133
        LD    A,L
        CP    C
        JP    Z,LBL130
LBL133: DEC   HL
        PUSH  BC
        LD    C,08H         ; 8
        CALL  bios_printChar
        LD    C,20H         ; 32 ' '
        CALL  bios_printChar
        LD    C,08H         ; 8
        CALL  bios_printChar
        POP   BC
        JP    LBL130
LBL134: LD    (HL),00H
        JP    LBL126
LBL135:  LD    C,05H         ; 5; [5]
LBL136: CALL  bios_beep_Old
        LD    B,0FFH                ; 255
        CALL  bios_delay_b
        CALL  bios_delay_b
        DEC   C
        JP    NZ,LBL136
        JP    LBL126
;----------------------------------------------------------------------

insertFile:
        CALL  prepareFileName
        LD    HL,fileDescName

        ; Запрашиваем дескриптор файла
        CALL  bios_fileLoadInfo
        LD    HL,(fileDescSize)
        EX    DE,HL         ; de = размер файла
        PUSH  DE
        LD    HL,(VAR01)    ; hl = адрес буфера

        ; Сканируем буфер, пока не найдем конец текста (0FFh)
LBL137: LD    A,(HL)
        INC   A
        JP    Z,LBL138
        INC   HL
        JP    LBL137

LBL138: POP   DE    ; de = размер файла
        PUSH  HL    ; hl = адрес конца буфера
        ADD   HL,DE
        EX    DE,HL
        LD    HL,(VAR03)
        LD    A,H
        CP    D
        JP    C,LBL135
        JP    NZ,LBL139
        LD    A,L
        CP    E
        JP    C,LBL135

        ; загрузка файла по адресу de
LBL139: POP   DE
        LD    HL,fileDescName
        CALL  bios_fileLoad2
        JP    C,LBL135
        JP    LBL126

;----------------------------------------------------------------------

LBL140: POP   DE
        LD    HL,0000H
        LD    (bios_vars.inverse),HL
        JP    LBL10

;----------------------------------------------------------------------

txtFile:        DB    "FILE:"

    IF STANDARD_EDITOR

inputBuf:       BLOCK 10, 00h
inputBufEnd:    DB    00H,00H,00H

; Дескриптор файла (запись в каталоге MXOS FAT8)
fileDescName:   BLOCK 10, 20h   ; имя, расширение и атрибуты
fileDescAddr:   DW    2020H     ; адрес загрузки
fileDescSize:   DW    2020H     ; размер
fileDescCRC:    DB    20H       ; контрольная сумма
                DB    20H       ; первый сектор

    ELSE

inputBuf:       BLOCK 16, 00h
inputBufEnd:    DB    00H,00H,00H

; Дескриптор файла (запись в каталоге FAT16)
fileDescName:   BLOCK 12, 20h   ; имя, расширение и атрибуты
                BLOCK 6, 0
fileDescAddr:   DW    2020H     ; адрес загрузки
                BLOCK 8, 0
fileDescSize:   DW    2020H     ; размер
                BLOCK 2, 0

    ENDIF

txtSpace12:     DB   "            ",00H

;----------------------------------------------------------------------

        ; Обработка кода нажатой клавиши
keyInput:
        CALL  bios_getch
        LD    C,A
        CP    03H               ; F4 - открыть файл
        JP    Z,keyHandler_F4
        CP    04H               ; F5 - сохранить файл на диск
        JP    Z,keyHandler_F5
        CP    05H               ; F6 - открыть файл и вставить его в конец документа
        JP    Z,keyHandler_F6
        JP    keyInput2

;----------------------------------------------------------------------

keyHandler_F4:
        CALL  inputFileName
        CALL  openFile
        JP    LBL3

keyHandler_F5:
        CALL  inputFileName
        CALL  saveFile
        JP    SUB1

keyHandler_F6:
        CALL  inputFileName
        CALL  insertFile
        JP    LBL3

;----------------------------------------------------------------------

        ; ???
        NOP

SUB63:
        CALL  SUB15
        LD    HL,BUFFER2
        RET
LBL145:
        EX    DE,HL
        LD    DE,REF29
        CALL  bios_fileNamePrepare
        LD    HL,(VAR01)
        EX    DE,HL
        LD    HL,REF29
        CALL  bios_fileLoad2
        JP    LBL2

        DB    00H,00H
REF29:  DB    00H,00H,00H,00H
