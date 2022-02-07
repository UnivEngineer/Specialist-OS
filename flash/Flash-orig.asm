;+---------------------------------------------------------------------------
; MXOS
; FLASH.cOM - драйвер флеш-диска ВВ55
;
; Драйвер имеет три функции (номер передаётся в регистре Е):
; 1 - записать сектор (256 байт, номер сектора в регистре D, адрес буфера в hl);
; 2 - считать сектор  (256 байт, номер сектора в регистре D, адрес буфера в hl);
; 3 - выдать размер диска (в секторах, в регистре А).
;
; 2022-01-24 Дизассемблировано и доработано SpaceEngineer
;
;----------------------------------------------------------------------------

; Функции DOS
fileGetSetDrive =  0C842h ; Получить/установить активное устройство
installDriver   =  0C860h ; Установить драйвер накопителя

; Порты
IO_KEYB_MODE  =  0FFE3h 
IO_PROG_A     =  0FFE4h 
IO_PROG_B     =  0FFE5h 
IO_PROG_C     =  0FFE6h 
IO_PROG_MODE  =  0FFE7h 

    ORG     0FA00h

    ; Установить драйвер для накопителя 7 ("H")
    ld      a, 7
    ld      hl, Driver
    jp      installDriver

    ; Выбрать накопитель 7 ("H")
    ld      e, 1
    ld      a, 7
    jp      fileGetSetDrive

Driver:
    ld      a, e
    cp      1
    ret z             ; Запись не поддерживается
    push    hl
    push    de
    push    bc

    ; Наcтройка порта ВВ55
    ld      a, 90h
    ld      (IO_PROG_MODE), a
    ld      a, 0Dh              ; ??? порт клавиатуры
    ld      (IO_KEYB_MODE), a

    ld      a, e
    cp      3
    jp z,   FuncGetSize
    cp      2
    jp nz,  Exit

    ; Чтение блока
    ; вход:
    ; d  - номер блока
    ; hl - адрес буфера в памяти
FuncRead:
    xor     a
    ld      e, a
ReadLoop:
    call    Read
    ld      (hl), a
    inc     hl
    inc     e
    jp z,   Exit
    jp      ReadLoop

    ; Определение объема накопителя
    ; выход:
    ; a - количество секторов
FuncGetSize:
    xor     a
    ld      b, a
    ld      D, a
    ld      e, 4 ; начальный байт в секторе
LbL3:
    call    Read
    cp      0FFh
    jp nz,  LbL4 ; прыгнуть, если не конец диска
    inc     b    ; увеличить счетчик байтов
LbL4:
    inc     e    ; следующий байт в секторе
    ld      a, e
    cp      0C0h
    jp nz,  LbL3
    ld      a, 0C0h
    sub     b

    ; Восстановление портов и выход
Exit:
    push    af
    ld      a, 0Ch
    ld      (IO_KEYB_MODE), a
    ld      a, 9bh
    ld      (IO_PROG_MODE), a
    pop     af
    pop     bc
    pop     de
    pop     hl
    ret

    ; Чтение данных
    ; вход:
    ; de - адрес в диске
    ; выход:
    ; a - данные
Read:
    ex      de, hl
    ld      (IO_PROG_B), hl
    ld      a, (IO_PROG_A)
    ex      de, hl
    ret

    END
