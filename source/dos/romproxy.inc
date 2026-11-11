; Локальный A: в DOS: чтение CPM16 в своём банке, запись/опрос через BIOS.
; B=1/2, DE=блок 256 байт, HL=буфер DOS. AF/BC/DE/HL разрушены; CF/A=ошибка.
dosSystemDriver:
    ld a,b
    IFDEF BANKED_DOS_BUILD
    cp 2
    jp z,dosSystemRead
    ENDIF
    ld (dosSystemRequest),a
    ld (dosSystemRequest+3),hl
    ex de,hl
    ld (dosSystemRequest+1),hl
    ld de,dosSystemRequest
    ld hl,BIOS_BANK_IO_START
    xor a
    call bankCall
    ret
dosSystemRequest: DS 5,0
    IFDEF BANKED_DOS_BUILD
; Локальный DOS READ использует тот же CPM16, что EXEC-клиент.
; Метаданные и полные данные не проходят через MAIN.
dosSystemRead:
    push hl
    ld hl,(DOS_DRIVER_REGISTRY)
    ld a,d
    cp h
    jp c,dosSystemReadBuffer
    jp nz,dosSystemReadBad
    ld a,e
    cp l
    jp nc,dosSystemReadBad
dosSystemReadBuffer:
    pop hl
    ld a,h
    cp 0FEh
    jp c,MEM_IO_SYSTEM
    jp nz,blockArgument
    ld a,l
    or a
    jp z,MEM_IO_SYSTEM
    jp blockArgument
dosSystemReadBad:
    pop hl
    jp blockArgument
    ENDIF
