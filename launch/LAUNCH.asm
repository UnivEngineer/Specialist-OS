;+---------------------------------------------------------------------------
; MXOS
; Запуск RKS файла
;
; 2021-01-27 Разработано SpaceEngineer
;----------------------------------------------------------------------------

    INCLUDE "../include/mxos.inc"
    
; Адрес загрузки монитора
MON_ADDR        =  0C000h 

; Адрес временного буфера загрузки монитора
MON_ADDR_TEMP   =  0E000h 

    ORG 0F800h

    ; В de передаётся адрес строки аргументов
    ld      a, (de)
    cp      20h
    jp c,   noargsRet ; аргумент не задан, выходим

    push    de
    ld      hl, txtLoading
    call    bios_printString
    pop     hl
    push    hl
    call    bios_printString
    pop     hl

    ; Загрузка программы
    ; Подготовка имени файла и переключение накопителя
    ld      de, nameBuffer
    call    bios_fileNamePrepare   ; hl = имя файла = строка аргументов
    ex      de, hl

    ; Загружаем файл
    ld      hl, nameBuffer
    call    bios_fileLoad
    jp c,   fileNotFoundRet

    ; Получаем адрес загрузки (= адрес запуска) файла в de
    ld      de, FILE_DESCRIPTOR.loadAddress
    add     hl, de
    ld      e, (hl)
    inc     hl
    ld      d, (hl)

    ; Помещаем в стек для передачи Монитору
    push    de

    ld      hl, txtLoading
    call    bios_printString
    ld      hl, txtMonitorPath
    call    bios_printString

    ; Загрузка Монитора
    ; Подготовка имени файла a:MON2.SYS
    ld      hl, txtMonitorPath
    ld      de, nameBuffer
    call    bios_fileNamePrepare

    ; Загружаем файл MON2.SYS во временный буфер - он затрет nc.com
    ld      hl, nameBuffer
    ld      de, MON_ADDR_TEMP   ; изменить адрес загрузки файла на de
    call    bios_fileLoad2      ; нужна исправленная функция! BIOS 4.50 и старше 
    jp c,   popFileNotFoundRet

    ; Получаем размер файла Монитора в de
    ld      de, FILE_DESCRIPTOR.size
    add     hl, de
    ld      e, (hl)
    inc     hl
    ld      d, (hl)

    ; hl = MON_ADDR_TEMP
    ; de = MON_ADDR_TEMP + размер монитора
    ld      hl, MON_ADDR_TEMP
    ex      de, hl
    add     hl, de
    ex      de, hl

    ; Копируем Монитор на адрес C000h. При этом он затрёт BIOS, и дисковые
    ; функции ОС станут недоступны. Поэтому и нужен был временный буфер.
    ld      bc, MON_ADDR
    call    memcpy

    ; Инициализируем STD контроллер цвета
    ld      a, 82h              ; порты A, C - вывод, порт B - ввод
    ld      (IO_KEYB_MODE), a
    ld      a, 0h               ; белый цвет
    ld      (IO_KEYB_C), a

    ; Запуск Монитора. Монитор сам переходит в режим STD, инициализируется,
    ; и запускает программу по адресу из вершины стека.
    jp      MON_ADDR

    ; Копироваение из hl в bc с увеличением адресов, пока hl не равно de
memcpy:
    ld      a, (hl)
    ld      (bc), a
    call    cmp_hl_de
    ret z
    inc     hl
    inc     bc
    jp      memcpy

cmp_hl_de:
    ld      a, l
    cp      e
    ret nz  
    ld      a, h
    cp      d
    ret

popFileNotFoundRet:
    pop     de ; восстанавливаем стек для правильного аварийного выхода
fileNotFoundRet:
    ld      hl, txtFileNotFound
    jp      bios_printString

noargsRet:
    ld      hl, txtNoArgs
    jp      bios_printString

    ; Esc + ) включает KOI-8 до конца выводимой строки
txtLoading:
    DB 0Ah,"LOADING ",0

txtFileNotFound:
    DB 0Ah,"FILE NOT FOUND",0

txtNoArgs:
    DB 0Ah,"USAGE: LAUNCH.COM <FILE.RKS>",0

txtMonitorPath:
    DB "A:MON2.MON",0

nameBuffer:
    BLOCK 10,0

    END
