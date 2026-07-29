;+---------------------------------------------------------------------------
; MXOS
; FORMAT.cOM
;
; 2022-01-14 Disassembled by SpaceEngineer
;----------------------------------------------------------------------------

; Функции DOS
getch            =  0C803h ; Ожидание ввода c клавиатуры
printchar        =  0C809h ; Вывод символа на экран
printString      =  0C818h ; Вывод строки на экран
fileGetSetDrive  =  0C842h ; Получить/установить активное устройство
diskDriver       =  0C863h ; Драйвер выбранного диска

buffer           =  0D100h ; Буфер

;----------------------------------------------------------------------------
    ORG 0D000h

    ld      a,(de)    ; В de передаётся адрес строки аргументов
    cp      20h
    jp nc,  LetterEntered

    ; Запрос буквы диска для форматирования
chooseDrive:
    ld      hl, str_ChoseDrive
    call    printString     ; Вывод сообщения 'cHOOSE DRIVE ? '
    call    getch           ; Ожидание нажатия клавиши
    cp      21h             ; сравнение c пробелом
    ret c   
    ld      c,a
    call    printchar

LetterEntered:
    ld      (str_A_Y_N),a   ; Заменим 'a' в строке на введённую букву
    sub     41h             ; Номер диска
    cp      08h             ; Максимальный номер диска = 7
    jp nc,  chooseDrive     ; Повтор запроса буквы, если неверная
    ld      b, a            ; Запомним номер диска в b

    ld      hl, str_Format
    call    printString     ; Вывод сообщения 'FORMAT b: Y/N'
    call    getch           ; Ожидание нажатия клавиши
    cp      59h             ; сравнение c 'Y'
    ret nz                  ; Выход, если не 'Y'

    ; Установим выбранный диск текущим
    ld      a, b            ; Номер диска в a
    ld      e, 01h
    call    fileGetSetDrive

    ; Выдать размер диска в a
    ld      e, 03h
    call    diskDriver
    ld      e, a    ; Помещаем размер диска в e
    dec     a

    ; Очистка буфера (e байт)
    ld     hl, buffer
clearbufLoop:
    ld      m, 0
    inc     l
    dec     e
    jp nz,  clearbufLoop

    ; Создание пустой структуры FAT (256 байт)
createFaTLoop:
    inc     a
    jp z,   WriteToDisk
    ld      m, 01h
    inc     l
    jp      createFaTLoop

    ; Запись FAT на диск
    ; d - номер сектора
    ; e - код операции
WriteToDisk:
    ld      de, 0001h   ; Запись сектора номер 0
    call    diskDriver

    ; Создание пустой структуры каталога (256 байт)
createcatLoop:
    ld      m, 0FFh
    inc     l
    jp nz,  createcatLoop

    ; Запись секторов c номера 3 по 1
    ld      d, 03h
WriteLoop:
    call    diskDriver
    dec     d
    jp nz,  WriteLoop

    ; Выход в ОС
    ret

; Данные

str_Format:
    DB 0Ah,"FORMAT "

str_A_Y_N:
    DB "A: Y/N ",0

str_ChoseDrive:
    DB 0Ah,"CHOOSE DRIVE ? ",0

    END
