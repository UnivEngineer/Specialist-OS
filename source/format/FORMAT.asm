;----------------------------------------------------------------------------
; MXOS
; FORMAT.COM
;
; Доработки:
; - ключи -R (RAM-диск), -Y (без подтверждения), диск в виде B:.
;   Например FORMAT.COM B: -R -Y; порядок ключей и диска произвольный.
;
; 2022-01-14 Дизассемблировано и переработано SpaceEngineer
;----------------------------------------------------------------------------

    INCLUDE "../include/mxos.inc"

SECTOR_SIZE = 256           ; размер сектора в байтах

    ORG 0000h

    xor     a
    ld      (bios_vars.koi7), a      ; Файловые утилиты работают в KOI-8.
    ; DE — строка аргументов, C=0 — диск пока не задан.
    ex      de, hl
    ld      c, 0
    jp      ReadParams

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
    jp      SearchExit

ReadParams:
    call    bios_argNext
    jp c,   ParamsDone
    ld      a, b
    cp      2               ; Диск B: и ключи -R/-Y занимают два байта.
    jp nz,  errParams
    ld      a, (de)
    cp      '-'
    jp z,   ReadSwitch
    call    bios_upperCase
    cp      'A'
    jp c,   errInvalidDrive
    cp      'I'
    jp nc,  errInvalidDrive
    ld      b, a
    ld      a, c
    or      a
    jp nz,  errParams       ; Второй диск не допускается.
    ld      c, b
    inc     de
    ld      a, (de)
    cp      ':'
    jp nz,  errInvalidDrive
    jp      ReadParams
ReadSwitch:
    inc     de
    ld      a, (de)
    call    bios_upperCase
    ld      de, v_FormatConfirmed
    cp      'Y'
    jp z,   SetSwitch
    cp      'R'
    jp nz,  errParams
    ld      de, v_FormatRAMdisk
SetSwitch:
    ld      a, 0FFh
    ld      (de), a
    jp      ReadParams
ParamsDone:
    ld      b, c
    ld      a, b
    or      a
    jp z,   chooseDrive
    jp      SearchExit

SearchExit:
    ld      a, (v_FormatConfirmed)
    or      a
    jp nz,  Confirmed

    ; Ключ -Y не задан — запрос подтверждения.
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

    ; Пока поддерживается прежняя геометрия MXOS-256, до 8192 секторов.
    ; Стандартные FAT12/16 и логические секторы 512 байт вводятся отдельно.
    push de
    ld hl,8192
    call bios_cmp_hl_de
    pop de
    jp c,errSize

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

    ; FAT округляется вверх; её буфер соответствует целому числу секторов.
    add     hl,hl
    ld      de,SECTOR_SIZE-1
    add     hl,de
    ld      de,SECTOR_SIZE
    call    bios_div_hl_de
    ld      (v_FatSectors),hl
    push    hl
    ld      h,l
    ld      l,0
    ld      (v_FatBytes),hl
    pop     hl

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
    jp c,   errIO

;----------------------------------------------------------------------------
; FAT

    ; FAT формируется по одному сектору, без буфера на весь том.
    ld hl,0
    ld (v_FatIndex),hl
    ld hl,(v_DataSectors)
    inc hl
    inc hl
    ld (v_FatEnd),hl
    ld a,(v_FatSectors)
    ld (v_FatRemaining),a
    ; Первый BAD: cluster-2+dataStart имеет младший байт FFh.
    ld a,(v_DataStartSector)
    ld b,a
    ld a,1
    sub b
    ld l,a
    ld h,0
    ld (v_NextBad),hl
    ld de,1
FormatFatSector:
    push de                  ; номер записываемого сектора
    call ClearSector
    ; Нулевой сектор уже описывает все свободные кластеры.
    ; Только последний сектор может содержать запас вне области данных.
    ld hl,(v_FatIndex)
    ex de,hl
    ld hl,(v_FatEnd)
    call bios_sub_hl_de
    ld de,SECTOR_SIZE / FAT_ITEM_SIZE
    call bios_cmp_hl_de
    jp nc,FormatFatHeader
    add hl,hl                ; смещение первого элемента запаса в байтах
    ex de,hl
    ld hl,buffer
    add hl,de
    push hl
    ld hl,SECTOR_SIZE
    call bios_sub_hl_de
    ex de,hl
    pop hl
    ld b,0FFh
    call memset
FormatFatHeader:
    ld hl,(v_FatIndex)
    ld a,h
    or l
    jp nz,FormatFatBads
    ld hl,0FFF8h
    ld (buffer),hl
    ld hl,0FFFFh
    ld (buffer+2),hl
FormatFatBads:
    ld a,(v_FormatRAMdisk)
    or a
    jp z,FormatFatWrite
    ld hl,(v_NextBad)
    ex de,hl
    ld hl,(v_FatEnd)
    call bios_cmp_hl_de
    jp c,FormatFatWrite
    jp z,FormatFatWrite
    ld hl,(v_FatIndex)
    ex de,hl
    call bios_sub_hl_de        ; расстояние до следующей BAD-записи
    ld a,h
    or a
    jp nz,FormatFatWrite
    ld a,l
    cp SECTOR_SIZE / FAT_ITEM_SIZE
    jp nc,FormatFatWrite
    add hl,hl
    ld de,buffer
    add hl,de
    ld (hl),FAT16_BAD & 0FFh
    inc hl
    ld (hl),FAT16_BAD >> 8
    ld hl,(v_NextBad)
    inc h                    ; следующая страница RAMD: ещё 256 кластеров
    ld (v_NextBad),hl
FormatFatWrite:
    pop de
    ld c,1
    call WriteBuffer
    jp c,errIO
    push de
    ld hl,(v_FatIndex)
    ld de,SECTOR_SIZE / FAT_ITEM_SIZE
    add hl,de
    ld (v_FatIndex),hl
    pop de
    ld a,(v_FatRemaining)
    dec a
    ld (v_FatRemaining),a
    jp nz,FormatFatSector

;----------------------------------------------------------------------------
; Корневой каталог

    ; Первый сектор корня содержит метку; остальные повторяют нулевой буфер.
    call ClearSector
    ld hl,v_VolLabel
    ld de,v_VolLabel+11
    ld bc,buffer
    call bios_memcpy_bc_hl
    ld a,8
    ld (buffer+11),a
    ld hl,(v_DirStartSector)
    ex de,hl
    ld c,1
    call WriteBuffer
    jp c,errIO
    push de
    call ClearSector
    pop de
    ld a,(v_DirSectors)
    dec a
    ld c,a
    call nz,WriteBuffer
    ret c

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
    ret c
    inc     de      ; следующий сектор
    ; Следующий сектор получает тот же подготовленный буфер.
    dec     c
    jp nz,  WriteBufferLoop
    ret

; HL/DE/B/AF разрушаются, остальные регистры сохранены.
ClearSector:
    ld hl,buffer
    ld de,SECTOR_SIZE
    ld b,0
    jp memset

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

errIO:
    ld hl,str_IO
    call bios_printString
    scf
    ret
errSize:
    ld hl,str_Size
    call bios_printString
    scf
    ret
str_IO: DB 0Ah,"I/O ERROR",0
str_Size: DB 0Ah,"TOO LARGE",0

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

errParams:
    ld      hl, str_Usage
    call    bios_printString
    scf
    ret

str_Usage:          DB 0Ah,"FORMAT.COM [A:-H:] [-R] [-Y]",0
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
v_FatIndex:         DW 0
v_FatEnd:           DW 0
v_FatRemaining:     DB 0
v_NextBad:          DW 0
v_FatBytes:         DW 0
v_DirBytes:         DW 0
v_DirSectors:       DW 0
v_DirStartSector:   DW 0
v_DataSectors:      DW 0
v_DataStartSector:  DW 0

buffer = 01000h ; Единственный секторный буфер, 256 байт

; Начало загрузочного сектора.
; Поля, отмеченные *, настраиваются этой программой.
v_HeaderStart:
    DB 0, 0, 0          ; BS_JmpBoot     - смещение  00h,   3 байт - jmp на загрузчик - не использзуется
    DB "MXOS256 "       ; BS_OEMName     - смещение  03h,   8 байт - сигнатура "MSWIN 4.1" или "MSDOS 5.0", но можно и свою :)
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

    ASSERT_DONT_FIT buffer
    END
