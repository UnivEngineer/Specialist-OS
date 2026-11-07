; Мост основной памяти для системного ROM/flash через FFFEh.
; Только аппаратный обмен: нет FAT, поиска файлов и загрузки DOS через DOS.
; В текущей сборке F240h-F43Ah, 507 байт; общая карта — MAINBIOS.asm.
; Обслуживает любые физические блоки A:, нужные DOS, а не только метаданные.
; Полные блоки файла в main идут локальным драйвером, минуя этот мост;
; кэш FAT/каталога и неполные фрагменты находятся в банке DOS.
; Промежуточного секторного буфера нет: чтение CPM16, запись AT29C — R4.
; A=FAR-код банка DOS, DE=его запрос {operation,block:word,buffer:word}.
; AF/BC/DE/HL разрушены, CF/A=код DOS. Нереентерабельно.
    INCLUDE "../include/mxos.inc"
    INCLUDE "../mem/diskabi.inc"
    INCLUDE "../mem/abi.inc"
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
    ld (io_vars.romBlock),hl
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
    call mainIoAddress
    ; AT29C010: два цикла по 128 байт; остальные AT29C: один по 256.
    ld hl,256
    ld a,(io_vars.romBlock+1)
    cp 2
    jp nz,mainIoWriteSize
    ld hl,128
mainIoWriteSize:
    ld (io_vars.romChunk),hl
    call mainIoWriteChunk
    ret c
    ld hl,(io_vars.romChunk)
    ld a,h
    or a
    ret nz
    jp mainIoWriteChunk
mainIoRead:
    call mainIoAddress
    ld a,(io_vars.romPage)
    or 080h
    ld (mainIoCopy),a
    ld hl,(io_vars.romTarget)
    ld (mainIoCopy+1),hl
    ld a,DOS_IO_PAGE
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

; Прямое программирование из банка DOS через регистровые порции R4.
; Между последовательными порциями нет CALL/PUSH служебного стека.
; Окно и SP main восстанавливаются; CF=ошибка DATA polling.
mainIoWriteChunk:
    ld hl,BANK_WINDOW
    ld de,BANK_WINDOW+32
    ld bc,io_vars.romSavedWindow
    call bios_memcpy_bc_hl
    ld hl,mainIoWriteWindow
    ld de,mainIoWriteWindowEnd
    ld bc,BANK_WINDOW
    call bios_memcpy_bc_hl
    ld hl,(io_vars.romSource)
    ld (mainIoReadSP+1),hl
    ld hl,(io_vars.romTarget)
    ld de,4
    add hl,de
    ld (mainIoWriteSP+1),hl
    ld hl,(io_vars.romChunk)
    ld a,h
    or a
    ld a,64
    jp nz,mainIoWriteLoops
    ld a,32
mainIoWriteLoops:
    ld (io_vars.romLoops),a
    ld a,(io_vars.romPage)
    ld (mainIoRomSelect+1),a
    ld hl,0
    add hl,sp
    ld (io_vars.romSavedSP),hl
    ; Разблокировка физической страницы AT29C; данные придут через R4.
    ld (IO_PAGE_ROM),a
    ld a,0AAh
    ld (05555h),a
    ld a,055h
    ld (02AAAh),a
    ld a,0A0h
    ld (05555h),a
    jp mainIoReadSP
mainIoWriteNext:
    ld hl,(mainIoReadSP+1)
    ld de,4
    add hl,de
    ld (mainIoReadSP+1),hl
    ld hl,(mainIoWriteSP+1)
    add hl,de
    ld (mainIoWriteSP+1),hl
    ld a,(io_vars.romLoops)
    dec a
    ld (io_vars.romLoops),a
    jp nz,mainIoReadSP
    ; Последний PUSH записал C по адресу следующего блока минус четыре.
    dec hl
    dec hl
    dec hl
    dec hl
    ld (io_vars.romTarget),hl
    dec hl
    dec hl
    dec hl
    dec hl
    ex de,hl
    ; Последний байт фиксируем отдельной записью: порядок двух шинных
    ; записей PUSH может отличаться в моделях CPU. DATA polling однозначен.
    ld a,c
    ld (de),a
    ld b,3
    ld l,0
mainIoWritePoll:
    ld a,(de)
    xor c
    and 080h
    jp z,mainIoWriteDone
    dec l
    jp nz,mainIoWritePoll
    dec b
    jp nz,mainIoWritePoll
    scf
mainIoWriteDone:
    ; Сохраняем результат в A, не трогая ещё не восстановленный стек.
    ld a,0
    adc a,a
    ld (io_vars.romCheck),a
    ld (IO_PAGE_RAM),a
    ld hl,(mainIoReadSP+1)
    ld (io_vars.romSource),hl
    ld hl,(io_vars.romSavedSP)
    ld sp,hl
    ld hl,io_vars.romSavedWindow
    ld de,io_vars.romSavedWindow+32
    ld bc,BANK_WINDOW
    call bios_memcpy_bc_hl
    ld a,(io_vars.romCheck)
    or a
    ret z
    ld a,1
    ld (bios_vars.diskWriteErr),a
    jp mainIoFailed
mainIoWriteWindow:
    DISP BANK_WINDOW
mainIoReadSP:
    ld sp,0
    ld a,DOS_IO_PAGE-1
    ld (IO_PAGE_RAMD),a
    pop bc
    pop de
mainIoRomSelect:
    ld a,0
    ld (IO_PAGE_ROM),a
mainIoWriteSP:
    ld sp,0
    push de
    push bc
    jp mainIoWriteNext
    ENT
mainIoWriteWindowEnd:
    ASSERT mainIoWriteWindowEnd-mainIoWriteWindow <= 32

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
mainIoRequest = io_vars.mainIoRequest
mainIoCopy = io_vars.mainIoCopy
mainIoEnd:
    ASSERT $ <= 0F600h
    END
