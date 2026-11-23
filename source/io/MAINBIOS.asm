; IO.SYS, сегмент MAIN. Смещения от BANK_BIOS_GATE (F000h) образуют
; внутренний ABI MEM/BIOS. Размер и конец сегмента задаёт System.layout.json.
;
; +0  FAR-список: A = банк источника, HL = адрес, BC = 1..256 байт.
; +3  Регистровый пакет: A = банк источника, C = номер функции BIOS.
;     Транспорт заранее помещает MX_REGS в BANK_FRAME_REGS. Поддерживаются
;     скалярные функции таблицы bbModes (режим 1) и TEXT_PRINT (режим 5).
;     При выполнении AF/BC/DE/HL содержат результат, BANK_FRAME_STATUS = 0.
;     Для остальных режимов A = DOS_ERR_UNSUPPORTED, CF = 1.
; +6  Завершение TEXT_PRINT: принимает результат в bbRegs, открывает командное
;     окно, снимает bbBusy и возвращает AF/BC/DE/HL из пакета.
; +9  Локальный список: HL = адрес в MAIN, BC = длина. Возвращает A = 0,
;     CF = 0 и открывает командное окно, результаты записей не собираются.
;
; Вызывающий гарантирует границы пакетов, номера операций и доступный стек.
    INCLUDE "../include/mxos.inc"
    INCLUDE "../mem/biosabi.inc"
    INCLUDE "../mem/abi.inc"
    INCLUDE "../../bin/banked/transport-imports.inc"
    ORG BANK_BIOS_GATE
bankBiosDispatch:
    jp biosCommandBatch
bankBiosPacketDispatch:
    jp bbEntry
bankBiosFinishDispatch:
    jp bbFinish
bankBiosBatchLocal:
    jp bbBatchLocal
bbEntry:
    ld (bbPage),a
    ld a,c
    ld (bbFunction),a
    push de
    ld e,a
    ld d,0
    ld hl,bbModes
    add hl,de
    ld a,(hl)
    pop de
    or a
    jp z,bbUnsupported
    ld (bbMode),a
bbFastFetch:
    dec a
    jp z,bbScalarFast
    ld hl,BANK_FRAME_REGS
    ld de,bbRegs
    ld b,MX_REGS
    call bankCopyBytes
    ld a,1
    ld (bbBusy),a
bbRegsReady:
    ld a,(bbMode)
    cp 5
    ld a,5
    jp z,bios_textDispatch
    jp bbUnsupported

bbScalarFast:
    ld de,BANK_FRAME_REGS
    ld hl,bbScalarDone
    push hl
    ld hl,(MEM_NATIVE_CALL)
    ld bc,6
    add hl,bc
    ld a,(bbFunction)
    ld c,a
    jp (hl)
bbScalarDone:
    push af
    ld a,(BANK_PROFILE)
    or a
    call nz,bankCommandWindow
    xor a
    ld (BANK_FRAME_STATUS),a
    pop af
    ret

bbFinish:
bbReturnRegs:
    call bankCommandWindow
    xor a
    ld (bbBusy),a
    ld (BANK_FRAME_STATUS),a
    ld hl,(bbRegs+6)
    push hl
    ld hl,(bbRegs+4)
    ld b,h
    ld c,l
    ld hl,(bbRegs+2)
    ex de,hl
    ld hl,(bbRegs)
    pop af
    ret

bbFetch:
    ld (bbCopy+1),hl
    ex de,hl
    ld (bbCopy+4),hl
    ld h,b
    ld l,c
    ld (bbCopy+6),hl
    ld a,(bbPage)
    ld (bbCopy),a
    xor a
    ld (bbCopy+3),a
    ld hl,bbCopy
    jp bankCopyFar

bbUnsupported:
    ld a,DOS_ERR_UNSUPPORTED
    scf
    ret

; 0 — нужен отдельный адаптер указателей/состояния; 1 — регистры;
; 2 — ASCIIZ; 5 — запрос TEXT_PRINT.
; Дисковые функции, memcpy/strcmp, установка драйверов/хуков и memtop
; нельзя исполнять над NEAR-адресами клиента в контексте main.
; Формат записи: номер функции, полный размер, маска регистров, слова
; регистров и встроенные данные. Нулевой номер завершает список. F000h
; возвращает CF=1, A=DOS_ERR_ARGUMENT при длине вне диапазона 1..256.
biosCommandBatch:
    push af
    dec bc
    ld a,b
    or a
    inc bc
    jp nz,bbBatchTooLarge
    pop af
    ld (bbPage),a
    ld de,bbBuffer
    push bc
    call bbFetch
    pop bc
    ld hl,bbBuffer
bbBatchLocal:
    push hl
    add hl,bc
    ld (bbPacketAddress),hl
    pop hl
bbBatchRecord:
    ld a,(hl)
    or a
    jp z,bbBatchDone
    ld (bbFunction),a
    inc hl
    ld a,(hl)
    inc hl
    push hl
    sub 2
    ld e,a
    ld d,0
    add hl,de
    ld (bbSource),hl
    pop hl
    DEFINE BATCH_REGS bbRegs
    INCLUDE "../common/batch-args.inc"
    UNDEFINE BATCH_REGS
    ld a,(bbFunction)
    ld c,a
    ld de,bbRegs
    ld hl,bbBatchNext
    push hl
    ld hl,(MEM_NATIVE_CALL)
    DUP 6
        inc hl
    EDUP
    jp (hl)
bbBatchNext:
    ld hl,(bbPacketAddress)
    ex de,hl
    ld hl,(bbSource)
    call bios_cmp_hl_de
    jp c,bbBatchRecord
bbBatchDone:
    call bankCommandWindow
    xor a
    ret

bbBatchTooLarge:
    pop af
    ld a,DOS_ERR_ARGUMENT
    scf
    ret

bbModes:
    DB 1    ; GETCH
    DB 0    ; TAPE_READ
    DB 1    ; PRINT_CHAR
    DB 0    ; TAPE_WRITE
    DB 0    ; INPUT
    DB 1    ; KEY_CHECK
    DB 1    ; PRINT_HEX_BYTE
    DB 2    ; PRINT_STRING_Z
    DB 1    ; KEY_SCAN
    DB 1    ; GET_CURSOR_POS
    DB 1    ; SET_CURSOR_POS
    DB 0    ; TAPE_LOAD
    DB 0    ; TAPE_SAVE
    DB 0    ; CALC_CS
    DB 1    ; PRINT_HEX_WORD
    DB 1    ; SET_MEM_TOP: scalar HL, used by persistent MEM reservations
    DB 1    ; GET_MEM_TOP: scalar HL result
    DB 1    ; PRINTER
    DB 0    ; INSTALL_DRIVER
    DB 0    ; DISK_DRIVER
    DB 1    ; PRINT_CHAR_RAW
    DB 1    ; PRINT_DEC_WORD
    DB 0    ; SET_RAMD_DRIVER
    DB 1    ; UPPERCASE
    DB 0    ; STRCMP
    DB 0    ; STRICMP
    DB 1    ; SUB_HL_DE
    DB 1    ; BEEP_TIMER
    DB 0    ; DISK_WRITE_CHECKED
    DB 0    ; ARG_NEXT
    DB 0    ; INSTALL_BLOCK_DRIVER
    DB 0    ; SET_INPUT_HOOK
    DB 0    ; BLOCK_IO
    DB 0    ; MEMCPY
    DB 0    ; FILE_BODY
    DB 1    ; GLYPH_CACHE
    DB 1    ; PRINT_STRING_Z_MAIN
    DB 1    ; FONT_RENDER
    DB 2    ; PRINT_STRING_RAW_Z
    DB 1    ; PRINT_STRING_RAW_Z_MAIN
    DB 0    ; PRINT_STRING_RAW_LIM
    DB 1    ; PRINT_STRING_RAW_LIM_MAIN
    DB 5    ; TEXT_PRINT
    DB 1    ; SET_FONT_ENCODING
    DB 1    ; COMMAND_BATCH
    DB 0    ; PRINT_STRING_RAW_PSEUDO: intercepted by MEM
bbModesEnd:
    ASSERT bbModesEnd-bbModes == MXOS_BIOS_PRINT_STRING_RAW_PSEUDO-0100h+1
bbBusy          = io_vars.bbBusy
bbFast          = io_vars.bbFast
bbPage          = io_vars.bbPage
bbFunction      = io_vars.bbFunction
bbMode          = io_vars.bbMode
bbPacketAddress = io_vars.bbPacketAddress
bbSource        = io_vars.bbSource
bbChunk         = io_vars.bbChunk
bbScanLeft      = io_vars.bbScanLeft
bbMore          = io_vars.bbMore
bbRegs          = io_vars.bbRegs
bbCopy          = io_vars.bbCopy
; Общий временный буфер: UI и физический A: не вложены друг в друга.
bbBuffer = BIOS_BANK_IO_BUFFER
bankBiosEnd:
    ASSERT $ <= BANK_BIOS_END
    END
