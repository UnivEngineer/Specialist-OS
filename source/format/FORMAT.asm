;----------------------------------------------------------------------------
; MXOS
; FORMAT.COM
;
; Доработки:
; - "тихий режим" по параметру Y (форматировать без cпроcа)
;    например FORMAT.COM B: Y
;
; 2022-01-14 Дизассемблировано и переработано SpaceEngineer
;----------------------------------------------------------------------------

    INCLUDE "../include/mxos.inc"

SECTOR_SIZE = 256           ; размер сектора в байтах

    ORG 0F100h

    ld      a, (de)         ; в de передаётся адрес строки аргументов
    cp      20h
    jp nc,  ReadParams      ; прыжок, если есть параметр

    ; Запрос буквы диска для форматирования
chooseDrive:
    ld      hl, str_ChoseDrive
    call    bios_printString; вывод сообщения 'CHOOSE DRIVE: '
    call    bios_getch      ; ожидание нажатия клавиши
    ld      c, a
    call    bios_printChar
    cp      21h             ; сравнение c пробелом
    jp c,   errAbort        ; выход в ОС, если меньше или равно
    call    bios_upperCase  ; превод буквы в верхний регистр
    ld      b, a            ; запомнить букву диска в b
    jp      ConfirmRequest

ReadParams:
    call    bios_upperCase  ; превод буквы в верхний регистр
    ld      b, a            ; запомнить букву диска в b

SearchLoop1:                ; поиск первого пробела в строке параметров
    ld      a, (de)
    cp      21h
    jp c,   SearchLoop2
    inc     de
    jp      SearchLoop1

SearchLoop2:                ; пропуск последующих пробелов
    ld      a, (de)
    cp      20h
    jp nz,  AnalyzeParam
    inc     de
    jp      SearchLoop2

AnalyzeParam:
    cp      21h
    jp c,   SearchExit      ; конец строки параметров
    call    bios_upperCase  ; превод буквы в верхний регистр
    cp      'Y'             ; если найден параметр 'Y', установим флаг "форматировать без подтверждения"
    jp nz,  AnalyzeParam1
    ld      a, 0FFh
    ld      (v_FormatConfirmed), a
    jp      SearchLoop1     ; продолжаем анализировать строку параметров
AnalyzeParam1:
    cp      'R'             ; если найден параметр 'R', установим флаг "форматировать как RAM-диск"
    jp nz,  SearchLoop1     ; иначе продолжаем анализировать строку параметров
    ld      a, 0FFh
    ld      (v_FormatRAMdisk), a
    jp      SearchLoop1     ; продолжаем анализировать строку параметров

SearchExit:
    ld      a, (v_FormatConfirmed)
    or      a
    jp nz,  Confirmed

    ; Параметр Y не найден - вывод запроса на подтверждение форматирования
ConfirmRequest:
    ld      a, b
    ld      (str_A_Y_N), a  ; заменить 'A' в строке сообщения на введённую букву
    ld      hl, str_Format
    call    bios_printString; вывод сообщения 'FORMAT <буква>: [Y/N]?'
    call    bios_getch      ; ожидание нажатия клавиши
    call    bios_upperCase  ; превод буквы в верхний регистр
    ld      c, a
    call    bios_printChar
    cp      'Y'             ; если найден параметр 'Y', переход к форматированию
    jp z,   Confirmed
    jp      errAbort        ; выход в ОС, если не 'Y/y'
    
Confirmed:
    ld      a, b            ; восстановить букву диска в a

    ; Буква диска в регистре a
Format:
    sub     41h             ; номер диска
    cp      08h             ; максимальный номер диска = 7
    jp nc,  errInvalidDrive ; выход в ОС, если неверный номер диска
    ld      b, a            ; запомнить номер диска в b

    ; Установить выбранный диск текущим
    ld      e, 01h          ; Номер диска в a
    call    bios_fileGetSetDrive

    ; Выдать размер диска в de
    ld      b, 3            ; режим 3 - получить размер
    call    bios_diskDriver ; de = размер в секторах

    ; Если de == 0, драйвер диска не установлен
    ld      a, d
    or      e
    jp z,   errNoDriver     ; выход в ОС, если нет драйвера

    ; Cохраняем количество секторов диска в структуру загрузочного сектора
    ex      hl, de
    ld      (v_TotalSectors), hl

    ; Если был параметр 'R', меняем метку тома на "RAM DISK"
    ld      a, (v_FormatRAMdisk)
    or      a               ; был ли параметр 'R'?
    jp z,   CalcParams      ; если не было, переходим

    ; Меняем метку тома
    push    hl
    ld      hl, str_RAM_DISK
    ld      de, str_RAM_DISK + 11
    ld      bc, v_VolLabel
    call    bios_memcpy_bc_hl
    pop     hl

CalcParams:

    ; Расчитываем размер таблицы fat
    add     hl, hl              ; hl * 2 - размер fat в байтах
    ld      (v_FatBytes), hl
    ld      de, SECTOR_SIZE     ; de = размер сектора в байтах
    call    bios_div_hl_de      ; hl = количество секторов fat
    ld      (v_FatSectors), hl  ; сохраняем значение

    ; Расчитываем номер первого сектора каталога
    inc     hl                      ; hl = размер fat + 1 загрузочный сектор
    ld      (v_DirStartSector), hl  ; сохраняем значение

    ; Расчитываем размер каталога
    ld      hl,(v_DirFiles)     ; количество дескрипторов в корневом каталоге
    ld      de, FILE_DESCR_SIZE ; размер дескриптора в байтах
    call    bios_mul_hl_de      ; размер каталога в байтах
    ld      (v_DirBytes), hl
    ld      de, SECTOR_SIZE
    call    bios_div_hl_de      ; hl = количество секторов каталога
    ld      (v_DirSectors), hl

    ; Расчитываем номер первого сектора области данных
    ex      hl, de
    ld      hl, (v_DirStartSector)
    add     hl, de                  ; hl = размер каталога + размер fat + 1 загрузочный сектор
    ld      (v_DataStartSector), hl ; сохраняем значение

    ; Расчитывем размер области данных
    ex      hl, de
    ld      hl, (v_TotalSectors); hl = количество секторов на диске
    call    bios_sub_hl_de      ; hl = количество секторов на диске минус все зарезервированные
    ld      (v_DataSectors), hl ; сохраняем значение

;----------------------------------------------------------------------------
; Загрузочный сектор

    ; Копируем заголовок загрузочного сектора в буфер
    ld      hl, v_HeaderStart
    ld      de, v_HeaderEnd
    ld      bc, buffer
    call    bios_memcpy_bc_hl

    ; Остальное заполняем байтом 0FFh
    ld      hl, buffer      + v_HeaderEnd - v_HeaderStart
    ld      de, SECTOR_SIZE - v_HeaderEnd + v_HeaderStart
    ld      b, 0FFh
    call    memset

    ; Записываем загрузочный сектор на диск
    ld      hl, buffer          ; адрес буфера
    ld      de, 0               ; сектор номер 0
    ld      c,  1               ; сколько секторов
    call    WriteBuffer

;----------------------------------------------------------------------------
; FAT

    ; Сколько ячеек fat нельзя использовать
    ld      hl, (v_DataStartSector) ; hl = сколько секторов заняты загрузочным сектором, fat и каталогом
    dec     hl                      ; первые две ячейки fat зарезервированы
    dec     hl
    add     hl, hl                  ; hl *= 2 - столько байт в конце таблицы fat нельзя использовать (сектор = кластер!)
    push    hl

    ; Обнуляем все ячейки fat (пустой кластер), кроме последних
    ld      hl, (v_FatBytes)    ; размер fat в байтах
    pop     de                  ; столько байт в конце таблицы fat нельзя использовать
    push    de
    call    bios_sub_hl_de      ; уменьшаем fat на столько
    ex      hl, de              ; de = сколько байт заполнять
    ld      hl, buffer          ; адрес буфера
    ld      b, 0                ; байт для заполнения
    call    memset

    ; Помещаем слово FFFFh (зарезервированный кластер) в последние ячейки fat
    pop     de                  ; столько байт осталось до конца таблицы fat
    ld      b, 0FFh             ; байт для заполнения
    call    memset

    ; Помещаем слово FFFFh (зарезервированный кластер) в первые две зарезервированнные ячейки fat
    ld      hl, 0FFFFh
    ld      (buffer),   hl
    ld      (buffer+2), hl

    ; Если это RAM-диск, помещаем слово FFF7h (плохой сектор) в ячейки fat,
    ; соответсвующие неполным секторам в конце 64 кб блоков
    ld      a, (v_FormatRAMdisk)
    or      a                   ; был ли параметр 'R'?
    call nz,MarkRamDiskBads

    ; Записываем FAT на диск
    ld      hl, (v_FatSectors)  ; количество секторов fat
    ld      c, l                ; количество секторов fat (считаем, что их меньше 256)
    ld      hl, buffer          ; адрес буфера
    ld      de, 1               ; начинаем с сектора номер 1
    call    WriteBuffer

;----------------------------------------------------------------------------
; Корневой каталог

    ; Создание пустого каталога (de байт 0FFh)
    ld      hl, (v_DirBytes)    ; размер каталога в байтах
    ex      hl, de
    ld      hl, buffer          ; адрес буфера
    ld      b, 0FFh             ; байт для заполнения
    call    memset

    ; Записываем каталог на диск
    ld      a, (v_DirSectors)
    ld      c, a                ; количество секторов каталога (считаем, что их меньше 256)
    ld      hl, (v_FatSectors)  ; количество секторов fat
    inc     hl                  ; количество секторов fat + 1 = начальный сектор каталога
    ex      hl, de              ; начальный сектор каталога
    ld      hl, buffer          ; адрес буфера
    call    WriteBuffer

    ; Выход в ОС
    ret

;----------------------------------------------------------------------------
; Запись буфера на диск
; c - сколько секторов
; de - номер первого сектора

WriteBuffer:
    ld      b,  1   ; режим 1 - запись
    ld      hl, buffer
WriteBufferLoop:
    call    bios_diskDriver
    inc     de      ; следующий сектор
    inc     h       ; седующий блок в памяти (размер сектора 256 байт)
    dec     c
    jp nz,  WriteBufferLoop
    ret

;----------------------------------------------------------------------------
; Пометить неполные секторы RAM-диска как плохие
; Формула вычисления номера сектора по номеру кластера
; sector = (cluster - 2) * BPB_SecPerClus + v_DataStartSector
; Обратная формула:
; cluster = (sector - v_DataStartSector) / BPB_SecPerClus + 2


MarkRamDiskBads:
    ; Вычисляем номер кластера, соответствующий первому плохому сектору на RAM-диске
    ld      hl, (v_DataStartSector)
    ld      de, 255 ; плохой сектор - последний в 64к банке
    ex      de, hl
    call    bios_sub_hl_de
    ; тут дожно быть hl = hl / BPB_SecPerClus, но у нас пока сектор = кластер
    inc     hl
    inc     hl

    ; Переводим номер кластера в адрес ячейки в буфере fat
    add     hl, hl  ; hl *= 2
    ld      de, buffer
    add     hl, de

    ; Приращение адреса для перехода к следующему плохому сектору
    ; -1 потому что в цикле один раз делается inc hl
    ld      de, 65536/256 * 2 - 1   ; = 511 байт

    ; В цикле помечаем плохие сектора каждой 64к банки
    ld      a, (bios_vars.ramPageCount) ; a = количество 64к банок RAM-диска
    ld      b, a                        ; счетчик цикла
MarkRamDiskBadsLoop:
    ld      (hl), FAT16_BAD & 0FFh
    inc     hl
    ld      (hl), FAT16_BAD >> 8
    add     hl, de
    dec     b
    jp nz,  MarkRamDiskBadsLoop
    ret

;----------------------------------------------------------------------------
; Заполнение памяти по адресу hl байтом b количеством байт de

memset:
    ld      (hl), b
    inc     hl
    dec     de
    ld      a, d
    or      e
    jp nz,  memset
    ret

;----------------------------------------------------------------------------
; Вывод сообщения 'INVALID DRIVE LETTER'

errInvalidDrive:
    ld      hl, str_InvalidDrive
    jp      bios_printString

;----------------------------------------------------------------------------
; Вывод сообщения 'NO DRIVER'

errNoDriver:
    ld      hl, str_NoDriver
    jp      bios_printString

;----------------------------------------------------------------------------
; Вывод сообщения 'DRIVE SIZE IS UNKNOWN'

;errSizeUnknown:
;    ld      hl, str_SizeUnknown
;    jp      bios_printString

;----------------------------------------------------------------------------
; Вывод сообщения 'ABORTING'

errAbort:
    ld      hl, str_Aborting
    jp      bios_printString

;----------------------------------------------------------------------------
; Данные

str_Format:         DB 0Ah,"Format drive "
str_A_Y_N:          DB "A: [Y/N]? ",0
str_ChoseDrive:     DB 0Ah,"Choose drive: ",0
str_InvalidDrive:   DB 0Ah,"Invalid drive letter",0
str_NoDriver:       DB 0Ah,"No driver",0
;str_SizeUnknown:    DB 0Ah,"Drive size is unkown",0
str_Aborting:       DB 0Ah,"Aborting",0
str_RAM_DISK:       DB "RAM DISK   "

v_FormatConfirmed:  DB 0
v_FormatRAMdisk:    DB 0
v_FatBytes:         DW 0
v_DirBytes:         DW 0
v_DirSectors:       DW 0
v_DirStartSector:   DW 0
v_DataSectors:      DW 0
v_DataStartSector:  DW 0

buffer = 0000h ; 0D100h ; Адрес буфера

; Начало загрузочного сектора.
; Поля, отмеченные *, настраиваются этой программой.
v_HeaderStart:
    DB 0, 0, 0          ; BS_JmpBoot     - смещение  00h,   3 байт - jmp на загрузчик - не использзуется
    DB "SPETSMX2"       ; BS_OEMName     - смещение  03h,   8 байт - сигнатура "MSWIN 4.1" или "MSDOS 5.0", но можно и свою :)
    DW SECTOR_SIZE      ; BPB_BytsPerSec - смещение  0Bh,   2 байт - размер сектора в байтах
    DB 1                ; BPB_SecPerClus - смещение  0Dh,   1 байт - сколько секторов в кластере
    DW 1                ; BPB_RsvdSecCnt - смещение  0Eh,   2 байт - сколько секторов в арезервированной (загрузочной) области
    DB 1                ; BPB_NumFATs    - смещение  10h,   1 байт - количество копий FAT
v_DirFiles:
    DW 64               ; BPB_RootEntCnt - смещение  11h,   2 байт - количество файлов в корневом каталоге (512 - стандрат для FAT16)
v_TotalSectors:
    DW 0            ; * ; BPB_TotSec16   - смещение  13h,   2 байт - сколько всего секторов на диске (16-битное значение)
    DB 0F8h             ; BPB_Media      - смещение  15h,   1 байт - допустимо F0h, F8h ... FFh, такое же значение должно быть в младших 8 битах FAT[0]
v_FatSectors:
    DW 0            ; * ; BPB_FATSz16    - смещение  16h,   2 байт - сколько секторов занимает FAT
    DW 0                ; BPB_SecPerTrk  - смещение  18h,   2 байт - количество секторов на дорожку
    DW 0                ; BPB_NumHeads   - смещение  1Ah,   2 байт - количество головок
    DB 0, 0, 0, 0       ; BPB_HiddSec    - смещение  1Ch,   4 байт - количество скрытых секторов перед этим разделом
    DB 0, 0, 0, 0       ; BPB_TotSec32   - смещение  20h,   4 байт - сколько всего секторов на диске (32-битное значение)
    DB 80h              ; BS_DrvNum      - смещение  24h,   1 байт - номер накопителя IBM PC: 0 для дисковода, 80h для жествого диска
    DB 0                ; BS_Reserved    - смещение  25h,   1 байт - зарезервировано для Windows NT
    DB 29h              ; BS_BootSig     - смещение  26h,   1 байт - расширенная сигнатура загрузочного сектора (29h): следующие три поля валидны
    DB 0, 0, 0, 0       ; BS_VolID       - смещение  27h,   4 байт - серийный номер тома
v_VolLabel:
    DB "NO NAME    "; * ; BS_VolLab      - смещение  2Bh,  11 байт - метка тома
    DB "FAT16   "       ; BS_FilSysType  - смещение  36h,   8 байт - сигнатура "FAT12   ", "FAT16   " или "FAT     "

v_HeaderEnd:            ; остальное заполняется байтом 0FFh

;----------------------------------------------------------------------------

    END
