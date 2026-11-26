NATIVE_RAMD_READ  = 0FFC0h
NATIVE_RAMD_WRITE = 0FFD0h

; Секторный адаптер встроенного блочного окна. Вызывается после проверки
; аргументов общим ROM/RAM-диспетчером, сохраняющим регистры.
; Вход DE=номер сектора, HL=буфер основной RAM.
; Выход: основная RAM включена. AF/BC/DE/HL/флаги разрушаются.
; Общее окно должно быть в блочном режиме.
ramDReadSector:
            ld c,d
            ld d,e
            ld e,0
            jp NATIVE_RAMD_READ
ramDWriteSector:
            ld c,d
            ld d,e
            ld e,0
            jp NATIVE_RAMD_WRITE
