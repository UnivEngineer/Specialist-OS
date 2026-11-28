; IO.SYS, сегмент MAIN. Смещения от BANK_BIOS_GATE (F000h) образуют
; внутренний ABI MEM/BIOS. Размер и конец сегмента задаёт System.layout.json.
;
; +0  FAR-список: A = банк источника, HL = адрес, BC = 1..256 байт.
; +3  Регистровый пакет: A = банк источника, C = номер функции BIOS.
;     Транспорт заранее помещает MX_REGS в BANK_FRAME_REGS. Поддерживаются
;     скалярные функции таблицы bbModes (режим 1).
;     При выполнении AF/BC/DE/HL содержат результат, BANK_FRAME_STATUS = 0.
;     Для остальных режимов A = DOS_ERR_UNSUPPORTED, CF = 1.
; +6  Частный вход настройки часов клавиатуры из DOS. CF=1 при тайм-ауте.
; +9  Локальный список: HL = адрес в MAIN, BC = длина. Короткая пачка из
;     банка приходит на стеке MAIN через TEXT. Возвращает A = 0, CF = 0,
;     результаты записей не собираются.
; +0Ch/+10h Пары слов SP для UPPERCASE и SUB_HL_DE.
; +14h JMP на совместимый посимвольный PRINT_STRING_LIM в BIOS.
; +17h/+1Ah Частные входы MEM для вывода байта и слова в HEX.
;
; Вызывающий гарантирует границы пакетов, номера операций и доступный стек.
    INCLUDE "../mem/biosabi.inc"
    ASSERT $ == BANK_BIOS_GATE
bankBiosDispatch:
    jp biosCommandBatch
bankBiosPacketDispatch:
    jp bbEntry
    ASSERT $ == BIOS_BANK_KEY_CLOCK_INIT
    jp ioKeyClockInit
bankBiosBatchLocal:
    jp bbBatchLocal
; Первое слово выбирает исторический вектор BIOS, второе возвращает управление
; в MEM. Допущенные обработчики не пишут в стек и завершаются через RET.
    MACRO BIOS_FAST_SLOT operation, vector
        ASSERT $ == BANK_BIOS_GATE + ((((operation & 255) + BIOS_FAST_SLOT_BIAS) >> 1) & BIOS_FAST_SLOT_MASK)
        DW vector,bankFastReturn
    ENDM
bbFastSlots:
    BIOS_FAST_SLOT MXOS_BIOS_UPPERCASE,upperCase
    BIOS_FAST_SLOT MXOS_BIOS_SUB_HL_DE,sub_hl_de
bbFastSlotsEnd:
    ASSERT $ == BIOS_BANK_GATE_ADDR+14h
    jp bbPrintStringLimLegacy
    ASSERT $ == BIOS_BANK_PRINT_HEX_BYTE
    jp printHexByte
    ASSERT $ == BIOS_BANK_PRINT_HEX_WORD
    jp printHexWord
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
    ld hl,(MEM_NATIVE_CALL)
    ld de,9
    add hl,de
    ld de,bbRegs
    jp (hl)
bbBatchNext:
    pop hl
    pop de
    push de
    call cmp_hl_de
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

bbPage          = io_vars.bbPage
bbFunction      = io_vars.bbFunction
bbRegs          = io_vars.bbRegs
bbCopy          = io_vars.bbCopy
; Общий временный буфер: UI и физический A: не вложены друг в друга.
bbBuffer = BIOS_BANK_IO_BUFFER
bankBiosEnd:
    ASSERT $ <= BANK_BIOS_END
