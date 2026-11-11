; Мост основной памяти для системного ROM/flash через FFFEh.
; Только аппаратный обмен: нет FAT, поиска файлов и загрузки DOS через DOS.
; Адрес F240h; точный конец — System.layout.json, карта — MAINBIOS.asm.
; Обслуживает любые физические блоки A:, нужные DOS, а не только метаданные.
; Полные блоки файла в main идут локальным драйвером, минуя этот мост;
; кэш FAT/каталога и неполные фрагменты находятся в банке DOS.
; Чтение CPM16; запись через общий верхний BIOS_DISK_BUFFER и драйвер A:.
; A=FAR-код банка инициатора (DOS/EXEC), DE=его запрос
; {operation,block:word,buffer:word}. Буфер находится в том же банке.
; AF/BC/DE/HL разрушены, CF/A=код DOS. Нереентерабельно.
    INCLUDE "../include/mxos.inc"
    INCLUDE "../mem/diskabi.inc"
    INCLUDE "../mem/abi.inc"
    INCLUDE "../../bin/banked/transport-imports.inc"
    ORG BIOS_BANK_IO_START
mainBlockIO:
    or a
    jp z,mainIoBad
    ld (io_vars.romBufferPage),a
    ex de,hl
    ld (mainIoCopy+1),hl
    ld a,(io_vars.romBufferPage)
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
    ; Границы физического носителя и локального буфера до переключения ROM.
    ld a,(mainIoRequest)
    dec a
    cp 2
    jp nc,mainIoDispatch
    ld b,3
    xor a
    call bios_blockIO
    jp c,mainIoFailed
    ex de,hl
    ld a,h
    ld (io_vars.romBlocksHi),a
    ex de,hl
    ld hl,(mainIoRequest+1)
    call bios_cmp_hl_de
    jp nc,mainIoBad
    ld hl,(mainIoRequest+3)
    ld a,h
    cp 0FEh
    jp c,mainIoDispatch
    jp nz,mainIoBad
    ld a,l
    or a
    jp nz,mainIoBad
mainIoDispatch:
    ld a,(mainIoRequest)
    cp 3
    jp z,mainIoQuery
    cp 0FFh
    jp z,mainIoQuery
    cp 2
    jp z,mainIoRead
    dec a
    jp nz,mainIoBad
    ; Прямую запись выполняем только для установленного AT29C-драйвера.
    ld b,0FFh
    xor a
    call bios_blockIO
    jp c,mainIoFailed
    ld a,b
    cp DISK_TECH_FLASH
    jp nz,mainIoFailed
    ; Сначала полный сектор в верхнюю MAIN, затем штатный AT29-драйвер.
    ld a,(io_vars.romBufferPage)
    ld (mainIoCopy),a
    ld hl,(mainIoRequest+3)
    ld (mainIoCopy+1),hl
    xor a
    ld (mainIoCopy+3),a
    ld hl,BIOS_DISK_BUFFER
    ld (mainIoCopy+4),hl
    ld hl,256
    ld (mainIoCopy+6),hl
    ld hl,mainIoCopy
    call bankCopySector
    jp c,mainIoFailed
    ld hl,(mainIoRequest+1)
    ex de,hl
    ld hl,BIOS_DISK_BUFFER
    ld b,1
    xor a
    call bios_blockIO
    ret nc
    jp mainIoFailed
mainIoRead:
    call mainIoAddress
    ld a,(io_vars.romPage)
    or 080h
    ld (mainIoCopy),a
    ld hl,(io_vars.romTarget)
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
mainIoAddress:
    ld hl,(mainIoRequest+3)
    ld (io_vars.romSource),hl
    ld hl,(mainIoRequest+1)
    ld a,l
    add a,a
    ld a,h
    rla
    ld (io_vars.romPage),a
    ld a,l
    and 07Fh
    ld h,a
    ld l,0
    ld (io_vars.romTarget),hl
    ret

; Геометрия A: копируется в DOS. B=FFh возвращает только BC, без текста;
; строковыми описаниями владеет реестр DOS.
mainIoQuery:
    ld b,a
    xor a
    call bios_blockIO
    jp c,mainIoFailed
    ld a,(mainIoRequest)
    inc a
    ret z                       ; FFh: только BC, HL=0, без копирования строки.
    push bc
    push de
    ld (mainIoCopy+1),hl
    xor a
    ld (mainIoCopy),a
    ld a,(io_vars.romBufferPage)
    ld (mainIoCopy+3),a
    ld hl,(mainIoRequest+3)
    ld (mainIoCopy+4),hl
    ld hl,DISK_INFO
    ld (mainIoCopy+6),hl
    ld hl,mainIoCopy
    call bankCopyFar
    pop de
    pop bc
    ld hl,(mainIoRequest+3)
    ret
mainIoRequest = io_vars.mainIoRequest
mainIoCopy = io_vars.mainIoCopy
mainIoEnd:
    ASSERT $ <= 0F600h
    END
