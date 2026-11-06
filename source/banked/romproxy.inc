; Локальный драйвер A: в DOS направляет только физический обмен в BIOS.
; B=1/2, DE=блок 256 байт, HL=буфер DOS. AF/BC/DE/HL разрушены; CF/A=ошибка.
dosSystemDriver:
    ld a,b
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
