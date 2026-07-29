;+---------------------------------------------------------------------------
; MXOS
; FORMAT.COM
;
; 2022-01-14 Дизассемблировано и доработано SpaceEngineer
;
; Доработки:
; - "тихий режим" по параметру Y (форматировать без cпроcа)
;    например FORMAT.COM B: Y
;
;----------------------------------------------------------------------------

    INCLUDE "../include/mxos.inc"

    ORG 0F100h

    ld      a, (de)         ; В de передаётся адрес строки аргументов
    cp      20h
    jp nc,  Readparams      ; Прыжок, если есть параметр

    ; Запрос буквы диска для форматирования
chooseDrive:
    ld      hl, str_ChoseDrive
    call    bios_printString; Вывод сообщения 'CHOOSE DRIVE: '
    call    bios_getch      ; Ожидание нажатия клавиши
    ld      c, a
    call    bios_printChar
    cp      21h             ; сравнение c пробелом
    jp c,   Abort           ; Выход в ОС, если меньше или равно
    ld      b, a            ; Запомнить букву диска в b
    jp      ConfirmRequest

Readparams:
    ld      b, a            ; Запомнить букву диска в b

SearchLoop1:                ; Поиск первого пробела в строке параметров
    ld      a,(de)
    cp      21h
    jp c,   SearchLoop2
    inc     de
    jp      SearchLoop1

SearchLoop2:                ; Пропуск последующих пробелов
    ld      a,(de)
    cp      20h
    jp nz,  SearchLoopExit
    inc     de
    jp      SearchLoop2

SearchLoopExit:     
    cp      'Y'             ; Если найден параметр 'Y', переход к форматированию
    jp z,   Confirmed

    ; Подтверждение форматирования
ConfirmRequest:
    ld      a, b
    ld      (str_A_Y_N),a   ; Заменить 'A' в строке сообщения на введённую букву
    ld      hl, str_Format
    call    bios_printString; вывод сообщения 'FORMAT <буква>: [Y/N]?'
    call    bios_getch      ; Ожидание нажатия клавиши
    ld      c, a
    call    bios_printChar
    cp      'Y'             ; сравнение c 'Y'
    jp nz,  Abort           ; Выход в ОС, если не 'Y'
    
Confirmed:
    ld      a, b            ; Восстановить букву диска в a

    ; Буква диска в регистре a
Format:
    sub     41h             ; Номер диска
    cp      08h             ; Максимальный номер диска = 7
    jp nc,  InvalidDrive    ; Выход, если неверный номер диска
    ld      b, a            ; Запомнить номер диска в b

    ; Установить выбранный диск текущим
    ld      a, b    ; Номер диска в a
    ld      e, 01h
    call    bios_fileGetSetDrive

    ; Выдать размер диска в a
    ld      e, 03h
    call    bios_diskDriver
    ld      e, a    ; Поместить размер диска в e
    dec     a

    ; Создание пустой структуры FAT (е байт нулей)
    ld      hl, buffer
CreateFATLoop1:
    ld      (hl), 0
    inc     l
    dec     e
    jp nz,  CreateFATLoop1

    ; Помещаем код 01h в элементы, соответствующие "плохим" секторам
    ; (у RAM-диска это последний, у ROM-диска 48 кб это все что выше 48 кб, и т.д.)
CreateFATLoop2:
    inc     a
    jp z,   WriteToDisk
    ld      (hl), 01h
    inc     l
    jp      CreateFATLoop2

    ; Запись FAT на диск
    ; d - номер сектора
    ; e - код операции
WriteToDisk:
    ld      de, 0001h  ; Запись сектора номер 0
    call    bios_diskDriver

    ; Создание пустой структуры каталога (256 байт)
CreateCatLoop:
    ld      (hl), 0FFh
    inc     l
    jp nz,  CreateCatLoop

    ; Запись секторов c номера 3 по 1
    ld      d, 03h
WriteLoop:
    call    bios_diskDriver
    dec     d
    jp nz,  WriteLoop

    ; Выход в ОС
    ret

Abort:
    ld      hl, str_Aborting
    jp      bios_printString     ; Вывод сообщения 'ABORTING'

InvalidDrive:
    ld      hl, str_InvalidDrive
    jp      bios_printString     ; Вывод сообщения 'INVALID DRIVE LETTER'

;----------------------------------------------------------------------------
; Данные

str_Format:
    DB 0Ah,"FORMAT "

str_A_Y_N:
    DB "A: [Y/N]? ",0

str_ChoseDrive:
    DB 0Ah,"CHOOSE DRIVE: ",0

str_InvalidDrive:
    DB 0Ah,"INVALID DRIVE LETTER",0

str_Aborting:
    DB 0Ah,"ABORTING",0

buffer = 0D100h ; Буфер

;----------------------------------------------------------------------------

    END
