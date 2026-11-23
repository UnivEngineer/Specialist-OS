mainIoRead:
    call mainIoAddress
    ld (mainIoCopy),a
    ld (mainIoCopy+1),hl
    ld a,(io_vars.romBufferPage)
    ld (mainIoCopy+3),a
    ld hl,(mainIoRequest+3)
    ld (mainIoCopy+4),hl
    ld hl,256
    ld (mainIoCopy+6),hl
    ld hl,mainIoCopy
    call bankCopySector
    ret nc
mainIoFailed:
    ld a,DOS_ERR_IO
    scf
    ret
mainIoBad:
    ld a,DOS_ERR_ARGUMENT
    scf
    ret
; Перевод линейного номера 256-байтного блока в ROM-банк и адрес окна.
; Для системных образов с линейными 32-КБ ROM-банками.
