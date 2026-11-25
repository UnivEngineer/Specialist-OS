; IO.SYS, сегмент MAIN. Смещения от BANK_BIOS_GATE (F000h) образуют
; внутренний ABI MEM/BIOS. Размер и конец сегмента задаёт System.layout.json.
;
; +0  FAR-список: A = банк источника, HL = адрес, BC = 1..256 байт.
; +3  Регистровый пакет: A = банк источника, C = номер функции BIOS.
;     Транспорт заранее помещает MX_REGS в BANK_FRAME_REGS. Поддерживаются
;     скалярные функции таблицы bbModes (режим 1).
;     При выполнении AF/BC/DE/HL содержат результат, BANK_FRAME_STATUS = 0.
;     Для остальных режимов A = DOS_ERR_UNSUPPORTED, CF = 1.
; +6  Резерв до фиксированного входа +9.
; +9  Локальный список: HL = адрес в MAIN, BC = длина. Короткая пачка из
;     банка приходит на стеке MAIN через TEXT. Возвращает A = 0, CF = 0,
;     результаты записей не собираются.
; +0Ch/+10h Пары слов SP для UPPERCASE и SUB_HL_DE.
; +14h Совместимый посимвольный PRINT_STRING_LIM.
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
    DS 3,0
bankBiosBatchLocal:
    jp bbBatchLocal
; Первое слово выбирает исторический вектор BIOS, второе возвращает управление
; в MEM. Допущенные обработчики не пишут в стек и завершаются через RET.
    MACRO BIOS_FAST_SLOT operation, vector
        ASSERT $ == BANK_BIOS_GATE + ((((operation & 255) + BIOS_FAST_SLOT_BIAS) >> 1) & BIOS_FAST_SLOT_MASK)
        DW vector,bankFastReturn
    ENDM
bbFastSlots:
    BIOS_FAST_SLOT MXOS_BIOS_UPPERCASE,bios_upperCase
    BIOS_FAST_SLOT MXOS_BIOS_SUB_HL_DE,bios_sub_hl_de
bbFastSlotsEnd:
    ASSERT $ == BIOS_BANK_GATE_ADDR+14h
; При прежнем FNT или перехваченном C809h каждый код идёт в printChar.
bbPrintStringLimLegacy:
    push af
    push bc
bbPrintStringLimLoop:
    ld a,b
    or c
    jp z,bbPrintStringLimDone
    push bc
    ld c,(hl)
    call bios_printChar
    pop bc
    inc hl
    dec bc
    jp bbPrintStringLimLoop
bbPrintStringLimDone:
    pop bc
    pop af
    ret
bbEntry:
    ld b,0
    ld hl,bbModes
    add hl,bc
    ld a,(hl)
    dec a
    jp nz,bbUnsupported

bbScalarFast:
    ld hl,bbScalarDone
    push hl
    ld hl,(MEM_NATIVE_CALL)
    ld de,6
    add hl,de
    ld de,BANK_FRAME_REGS
    jp (hl)
bbScalarDone:
    push af
    xor a
    ld (BANK_FRAME_STATUS),a
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
; 2 — ASCIIZ.
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
    push hl                 ; Конец пачки хранится в кадре вызова.
    add hl,bc
    ex (sp),hl
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
    ex (sp),hl              ; Следующая запись лежит под адресом bbBatchNext.
    DEFINE BATCH_REGS bbRegs
    INCLUDE "../common/batch-args.inc"
    UNDEFINE BATCH_REGS
    ld a,(bbFunction)
    ld c,a
    ld hl,bbBatchNext
    push hl
    ld b,0
    ld hl,bbModes
    add hl,bc
    ld a,(hl)
    ld hl,(MEM_NATIVE_CALL)
    ld de,9
    add hl,de
    ld de,bbRegs
    jp (hl)
bbBatchNext:
    pop hl
    pop de
    push de
    call bios_cmp_hl_de
    jp c,bbBatchRecord
bbBatchDone:
    pop hl
    xor a
    ret

bbBatchTooLarge:
    pop af
    ld a,DOS_ERR_ARGUMENT
    scf
    ret

bbModes:
    MACRO BB_FIXED_COMMAND command, vector
        ASSERT $-bbModes == command
        ASSERT (vector & 0FF80h) == (bios_printChar & 0FF80h)
        DB 080h | (vector & 07Fh)
    ENDM
    DB 1    ; GETCH
    DB 0    ; TAPE_READ
    BB_FIXED_COMMAND MXOS_COMMAND_BIOS_PRINT_CHAR,bios_printChar
    DB 0    ; TAPE_WRITE
    DB 0    ; INPUT
    DB 1    ; KEY_CHECK
    BB_FIXED_COMMAND MXOS_COMMAND_BIOS_PRINT_HEX_BYTE,bios_printHexByte
    DB 2    ; PRINT_STRING_Z
    DB 1    ; KEY_SCAN
    BB_FIXED_COMMAND MXOS_COMMAND_BIOS_GET_CURSOR_POS,bios_getCursorPos
    BB_FIXED_COMMAND MXOS_COMMAND_BIOS_SET_CURSOR_POS,bios_setCursorPos
    DB 0    ; TAPE_LOAD
    DB 0    ; TAPE_SAVE
    DB 0    ; CALC_CS
    BB_FIXED_COMMAND MXOS_COMMAND_BIOS_PRINT_HEX_WORD,bios_printHexWord
    BB_FIXED_COMMAND MXOS_COMMAND_BIOS_SET_MEM_TOP,bios_setMemTop
    BB_FIXED_COMMAND MXOS_COMMAND_BIOS_GET_MEM_TOP,bios_getMemTop
    DB 1    ; PRINTER
    DB 0    ; INSTALL_DRIVER
    DB 0    ; DISK_DRIVER
    BB_FIXED_COMMAND MXOS_COMMAND_BIOS_PRINT_CHAR_RAW,bios_printCharRaw
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
    DB 1    ; PRINT_STRING_LIM_MAIN
    DB 1    ; SET_FONT_ENCODING
    DB 1    ; COMMAND_BATCH
    DB 0    ; PRINT_STRING_RAW_PSEUDO: перехватывает MEM
    DB 0    ; PRINT_STRING_LIM: перехватывает MEM
bbModesEnd:
    ASSERT bbModesEnd-bbModes == MXOS_BIOS_PRINT_STRING_LIM-0100h+1
bbPage          = io_vars.bbPage
bbFunction      = io_vars.bbFunction
bbRegs          = io_vars.bbRegs
bbCopy          = io_vars.bbCopy
; Общий временный буфер: UI и физический A: не вложены друг в друга.
bbBuffer = BIOS_BANK_IO_BUFFER
bankBiosEnd:
    ASSERT $ <= BANK_BIOS_END
    END
