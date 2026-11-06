; Мост основной памяти для системного ROM/flash через FFFEh.
; Только аппаратный обмен: нет FAT, поиска файлов и загрузки DOS через DOS.
; A=страница DOS, DE=её запрос {operation,block:word,buffer:word}.
; AF/BC/DE/HL разрушены, CF/A=код DOS. Нереентерабельно.
    INCLUDE "../include/mxos.inc"
    INCLUDE "diskabi.inc"
    INCLUDE "../../bin/banked/transport-imports.inc"
    ORG BIOS_BANK_IO_START
mainBlockIO:
    cp DOS_IO_PAGE
    jp nz,mainIoBad
    ex de,hl
    ld (mainIoCopy+1),hl
    ld a,DOS_IO_PAGE
    ld (mainIoCopy),a
    xor a
    ld (mainIoCopy+3),a
    ld hl,mainIoRequest
    ld (mainIoCopy+4),hl
    ld hl,5
    ld (mainIoCopy+6),hl
    ld hl,mainIoCopy
    call bankCopyFar
    jp c,mainIoBad
    ld a,(mainIoRequest)
    cp 3
    jp z,mainIoQuery
    cp 0FFh
    jp z,mainIoQuery
    cp 2
    jp z,mainIoTransfer
    dec a
    jp nz,mainIoBad
    ld hl,(mainIoRequest+3)
    ld (mainIoCopy+1),hl
    ld hl,BIOS_BANK_IO_BUFFER
    ld (mainIoCopy+4),hl
    call mainIoCopyBlock
    jp c,mainIoBad
mainIoTransfer:
    ld a,(mainIoRequest)
    ld b,a
    ld hl,(mainIoRequest+1)
    ex de,hl
    ld hl,BIOS_BANK_IO_BUFFER
    xor a
    call bios_blockIO
    jp c,mainIoFailed
    ld a,(mainIoRequest)
    cp 1
    jp z,mainIoSuccess
    xor a
    ld (mainIoCopy),a
    ld a,DOS_IO_PAGE
    ld (mainIoCopy+3),a
    ld hl,BIOS_BANK_IO_BUFFER
    ld (mainIoCopy+1),hl
    ld hl,(mainIoRequest+3)
    ld (mainIoCopy+4),hl
    call mainIoCopyBlock
    jp c,mainIoBad
mainIoSuccess:
    xor a
    ret
mainIoFailed:
    ld a,DOS_ERR_IO
    scf
    ret
mainIoBad:
    ld a,DOS_ERR_ARGUMENT
    scf
    ret
; Описание физического A: приходит от действительно установленного BIOS
; драйвера. Его NEAR-указатель заменяется копией в буфере DOS.
mainIoQuery:
    ld b,a
    xor a
    call bios_blockIO
    jp c,mainIoFailed
    push bc
    push de
    ld (mainIoCopy+1),hl
    xor a
    ld (mainIoCopy),a
    ld a,DOS_IO_PAGE
    ld (mainIoCopy+3),a
    ld hl,(mainIoRequest+3)
    ld (mainIoCopy+4),hl
    ld hl,64
    ld a,(mainIoRequest)
    cp 3
    jp nz,mainIoQuerySize
    ld hl,DISK_INFO
mainIoQuerySize:
    ld (mainIoCopy+6),hl
    ld hl,mainIoCopy
    call bankCopyFar
    pop de
    pop bc
    ld hl,(mainIoRequest+3)
    ret
mainIoCopyBlock:
    ld hl,256
    ld (mainIoCopy+6),hl
    ld hl,mainIoCopy
    jp bankCopyFar
mainIoRequest: DS 5,0
mainIoCopy: DS 8,0
mainIoEnd:
    ASSERT $ <= BIOS_BANK_IO_BUFFER
    END
