;----------------------------------------------------------------------------
; MXOS
; FORMAT.COM
;
; Доработки:
; - "тихий режим" по параметру Y (форматировать без cпроcа)
;    например FORMAT.COM B: Y
;
; 2022-01-14 Дизассемблировано и доработано SpaceEngineer
;----------------------------------------------------------------------------

    INCLUDE "../include/mxos.inc"

    ORG 0F100h

    ld      a, (de)         ; В de передаётся адрес строки аргументов
    cp      20h
    jp nc,  ReadParams      ; Прыжок, если есть параметр

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

ReadParams:
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
    ld      (str_A_Y_N), a  ; Заменить 'A' в строке сообщения на введённую букву
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
    push    af              ; И в стеке

    ; Установить выбранный диск текущим
    ld      e, 01h          ; Номер диска в a
    call    bios_fileGetSetDrive

    ; Выдать размер диска в de
    ld      b, 3            ; режим 3 - получить размер
    call    bios_diskDriver ; de = размер в кластерах

    ; Вычисляем количество рабочих и "плохих" кластеров
    ld      hl, FAT_SIZE / FAT_ITEM_SIZE
    call    sub_hl_de   ; hl = количество кластеров в fat минус количество кластеров на диске
    push    hl

    ; Обнуляем рабочую часть fat (de * CLUSTER_SIZE слов 0000h)
                        ; de = размер в кластерах
    ld      hl, buffer  ; hl = адрес буфера
    ld      bc, 0       ; bc = слово для заполнения
    call    memset

    ; Помещаем слово 0001h ("плохой" сектор) в оставшиеся ячейки fat
    ; (у ROM-диска 48 кб это все что выше 48 кб, и т.д.)
                        ; hl = адрес буфера (продолжаем предыдущий)
    pop     de          ; de = количество кластеров в fat минус количество кластеров на диске
    ld      bc, 1       ; bc = слово для заполнения
    call    memset

    ; Помещаем слово 0001h ("плохой" сектор) в зарезервированные ячейки fat, соответсвующие
    ; самой fat и корневому каталогу
    ld      hl, buffer                      ; hl = адрес буфера
    ld      de, FAT_CLUSTERS + DIR_CLUSTERS ; de = количество самой fat и корневого каталога
    ld      bc, 1                           ; bc = слово для заполнения
    call    memset

    ; Помещаем слово 0001h ("плохой" сектор) в ячейки fat, соответсвующие
    ; неполным секторам в конце 64 кб блоков, если это RAM-диск
    pop     af  ; a = номер диска
    cp      1   ; RAM-диск это диск 1 ("B:")
    call z, MarkRamDiskBads

    ; Запись FAT на диск
    ld      hl, buffer
    ld      de, 0               ; начинаем с кластера номер 0
    ld      c,  FAT_CLUSTERS    ; сколько кластеров
    call    WriteBuffer

    ; Создание пустого каталого каталога (DIR_SIZE байт 0FFh)
    ld      hl, buffer                      ; hl = адрес буфера
    ld      de, DIR_SIZE / FAT_ITEM_SIZE    ; de = количество секторов
    ld      bc, 0FFFFh                      ; bc = слово для заполнения
    call    memset

    ; Запись каталога на диск
    ld      hl, buffer
    ld      de, FAT_CLUSTERS ; начинаем с кластера номер FAT_CLUSTERS
    ld      c,  DIR_CLUSTERS ; сколько кластеров
    call    WriteBuffer

    ; Выход в ОС
    ret

;----------------------------------------------------------------------------

; Запись буфера на диск
; c - сколько кластеров
; de - номер первого кластера
WriteBuffer:
    ld      b,  1   ; режим 1 - запись
    ld      hl, buffer
WriteBufferLoop:
    call    bios_diskDriver
    inc     de      ; следующий кластер
    inc     h       ; седующий блок в памяти (размер кластера 256 байт)
    dec     c
    jp nz,  WriteBufferLoop
    ret

; Пометить неполные сектора RAM-диска как "бэды"
MarkRamDiskBads:
    ld      b,  FAT_CLUSTERS / FAT_ITEM_SIZE    ; b = количество 64кб банок RAM-диска
    ld      hl, buffer + 255 * FAT_ITEM_SIZE    ; hl = адрес первого "бэда" в буфере fat
    ld      de, 256 * FAT_ITEM_SIZE - 1         ; de = приращение адреса для перехода к следующему "бэду"
MarkRamDiskBadsLoop:
    ld      (hl), 1
    inc     hl
    ld      (hl), 0
    add     hl, de
    dec     b
    jp nz,  MarkRamDiskBadsLoop
    ret

; Заполнение памяти по адресу hl словом bc количеством слов de
memset:
    ld      a, d
    or      e
    ret     z   ; если de == 0, выходим
memsetLoop:
    ld      a, c
    ld      (hl), a
    inc     hl
    ld      a, b
    ld      (hl), a
    inc     hl
    dec     de
    ld      a, d
    or      e
    jp nz,  memsetLoop
    ret

; hl = hl - de
sub_hl_de:
    ld    a, l
    sub   e
    ld    l, a
    ld    a, h
    sbc   d
    ld    h, a
    ret

; Вывод сообщения 'ABORTING'
Abort:
    ld      hl, str_Aborting
    jp      bios_printString

    ; Вывод сообщения 'INVALID DRIVE LETTER'
InvalidDrive:
    ld      hl, str_InvalidDrive
    jp      bios_printString

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

buffer = 0000h ; 0D100h ; Адрес буфера

;----------------------------------------------------------------------------

    END
