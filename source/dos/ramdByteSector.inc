; Секторный адаптер встроенного побайтового ABI. Вызывается после проверки
; аргументов из общего ROM/RAM-диспетчера, который сохраняет регистры.
; Вход DE=сектор, HL=буфер; общее окно в байтовом режиме.
; До установки RAMD.COM B=0 восстанавливает именно этот режим.
; Разрушает AF/BC/DE/HL и флаги.
; Два цикла по 256 байт: чтение 110, запись 97 тактов/байт с CALL/RET;
; на байт одно чтение, одна запись данных и два переключения маппера.
ramDByteRead:
            ld      b, d
            ld      d, e
            ld      e, 0
            ex      de, hl
ramDByteReadLoop:
            ld      a, b
            call    bios_RAMDRead
            ld      a, c
            ld      (de), a
            inc     de
            inc     l
            jp nz,  ramDByteReadLoop
            ret
ramDByteWrite:
            ld      b, d
            ld      d, e
            ld      e, 0
            ex      de, hl
ramDByteWriteLoop:
            ld      a, (de)
            ld      c, a
            ld      a, b
            call    bios_RAMDWrite
            inc     de
            inc     l
            jp nz,  ramDByteWriteLoop
            ret
