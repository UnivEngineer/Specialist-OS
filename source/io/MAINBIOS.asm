; Внутренняя карта IO.SYS основной памяти.
; Файл с F000h загружается непрерывно; точный конец — System.layout.json.
; Адреса/размеры текущей сборки уточняет bin/System.layout.json.
;   F000-F23F - [576 B] MAINBIOS.asm: принимающий адаптер BIOS
;   F240-...  - MAINIO.asm: физический мост A: <-> DOS
; Файловые переходники, загрузчик тела, RAW-маршрутизатор и границы кэша
; компонуются в BIOS.SYS ниже области шрифта, а не входят в IO.SYS.
; Общие публичные JMP устанавливаются bootstrap из MEM.SYS.
; LOCALBLOCK (с F480h), FLASHSYS (F8C0h), H: (FA00h) не входят в IO.SYS.
; Рабочие переменные и буфер текста находятся отдельно от файла:
; IO_VARIABLES и BIOS_BANK_IO_BUFFER объявлены в mxos.inc.
;
; Адаптер банковых клиентов к BIOS основной памяти.
; A=страница клиента, BC=номер группы/функции, DE=NEAR MX_REGS клиента.
; Регистры результата возвращаются в CPU и в исходный пакет. SP/owner/home
; восстанавливает транспорт. Код BIOS не видит указателей чужой страницы.
; Один активный запрос: повторный вход возвращает ACCESS без порчи внешнего.
    INCLUDE "../include/mxos.inc"
    INCLUDE "../mem/biosabi.inc"
    INCLUDE "../mem/abi.inc"
    INCLUDE "../../bin/banked/transport-imports.inc"
    ORG BANK_BIOS_GATE
bankBiosDispatch:
    jp bbSlowEntry
bankBiosPacketDispatch:
    jp bbFastEntry
bankBiosFinishDispatch:
    jp bbFinish
bbSlowEntry:
    push af
    ld a,(bbBusy)
    or a
    jp nz,bbReentered
    ; A уже равен нулю после проверки bbBusy.
    ld (bbFast),a
    pop af
    jp bbEntry
bbFastEntry:
    push af
    ld a,(bbBusy)
    or a
    jp nz,bbReentered
    ld a,1
    ld (bbFast),a
    pop af
bbEntry:
    ld (bbPage),a
    ld a,b
    cp MXOS_GROUP_BIOS
    jp nz,bbUnsupported
    ld a,c
    cp bbModesEnd-bbModes
    jp nc,bbUnsupported
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
    ld a,(bbFast)
    or a
    jp nz,bbFastFetch
    ex de,hl
    ld (bbPacketAddress),hl
    ld bc,MX_REGS
    call bbCheckRange
    jp c,bbBadArgument
    ld a,1
    ld (bbBusy),a
    ld de,bbRegs
    call bbFetch
    jp c,bbCopyError
    jp bbRegsReady
bbFastFetch:
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
    ld a,(bbMode)
    dec a
    jp z,bbInvoke
    ld hl,(bbRegs)
    ld (bbSource),hl
    dec a ; После проверки скалярного пути A уже равно bbMode-1.
    jp z,bbString
    jp bbUnsupported

; Скалярные вызовы: адрес экрана и координаты — значения, не NEAR-буфер.
bbInvoke:
    ld a,(bbFunction)
    ld c,a
    ld b,MXOS_GROUP_BIOS
    ld de,bbRegs
    call mx_call
bbFinish:
    ld a,(bbFast)
    or a
    jp nz,bbReturnRegs
    ; Возвращаем пакет до восстановления AF исполнителя.
    ld hl,bbRegs
    ld (bbCopy+1),hl
    xor a
    ld (bbCopy),a
    ld a,(bbPage)
    ld (bbCopy+3),a
    ld hl,(bbPacketAddress)
    ld (bbCopy+4),hl
    ld hl,MX_REGS
    ld (bbCopy+6),hl
    ld hl,bbCopy
    call bankCopyFar
    call bankCommandWindow
    jp c,bbCopyError
bbReturnRegs:
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

; Строки печатаются пачками до 256 байт. Нулевой байт не пересылается
; отдельно, длинная строка не ограничена размером временного буфера.
; HL результата остаётся указателем исходной страницы на завершающий нуль.
bbString:
    ld a,6
    jp bios_textDispatch

; HL=NEAR начала, BC=размер; CF=диапазон пересекает хвост/оборотов адреса.
; HL/BC сохранены, AF/DE разрушены. Копирование никогда не читает за F800h.
bbCheckRange:
    push hl
    add hl,bc
    jp c,bbRangeDone
    ld de,BANK_CLIENT_LIMIT
    ld a,d
    cp h
    jp nz,bbRangeDone
    ld a,e
    cp l
bbRangeDone:
    pop hl
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

bbReentered:
    pop af
    ld a,DOS_ERR_ACCESS
    scf
    ret
bbCopyError:
    xor a
    ld (bbBusy),a
bbBadArgument:
    ld a,DOS_ERR_ARGUMENT
    scf
    ret
bbUnsupported:
    ld a,DOS_ERR_UNSUPPORTED
    scf
    ret

; 0 — нужен отдельный адаптер указателей/состояния; 1 — регистры;
; 2 — ASCIIZ; 5 — запрос TEXT_PRINT.
; Дисковые функции, memcpy/strcmp, установка драйверов/хуков и memtop
; нельзя исполнять над NEAR-адресами клиента в контексте main.
bbModes:
    DB 1 ; GETCH
    DB 0 ; TAPE_READ
    DB 1 ; PRINT_CHAR
    DB 0 ; TAPE_WRITE
    DB 0 ; INPUT
    DB 1 ; KEY_CHECK
    DB 1 ; PRINT_HEX_BYTE
    DB 2 ; PRINT_STRING_Z
    DB 1 ; KEY_SCAN
    DB 1 ; GET_CURSOR_POS
    DB 1 ; SET_CURSOR_POS
    DB 0 ; TAPE_LOAD
    DB 0 ; TAPE_SAVE
    DB 0 ; CALC_CS
    DB 1 ; PRINT_HEX_WORD
    DB 0 ; SET_MEM_TOP
    DB 0 ; GET_MEM_TOP
    DB 1 ; PRINTER
    DB 0 ; INSTALL_DRIVER
    DB 0 ; DISK_DRIVER
    DB 1 ; PRINT_CHAR_RAW
    DB 1 ; PRINT_DEC_WORD
    DB 0 ; SET_RAMD_DRIVER
    DB 1 ; UPPERCASE
    DB 0 ; STRCMP
    DB 0 ; STRICMP
    DB 1 ; SUB_HL_DE
    DB 1 ; BEEP_TIMER
    DB 0 ; DISK_WRITE_CHECKED
    DB 0 ; ARG_NEXT
    DB 0 ; INSTALL_BLOCK_DRIVER
    DB 0 ; SET_INPUT_HOOK
    DB 0 ; BLOCK_IO
    DB 0 ; MEMCPY
    DB 0 ; FILE_BODY
    DB 1 ; GLYPH_CACHE
    DB 1 ; PRINT_STRING_Z_MAIN
    DB 1 ; FONT_RENDER
    DB 2 ; PRINT_STRING_RAW_Z
    DB 1 ; PRINT_STRING_RAW_Z_MAIN
    DB 0 ; PRINT_STRING_RAW_LIM
    DB 1 ; PRINT_STRING_RAW_LIM_MAIN
    DB 5 ; TEXT_PRINT
    DB 1 ; SET_FONT_ENCODING
bbModesEnd:
    ASSERT bbModesEnd-bbModes == MXOS_BIOS_SET_FONT_ENCODING-0100h+1
bbBusy = io_vars.bbBusy
bbFast = io_vars.bbFast
bbPage = io_vars.bbPage
bbFunction = io_vars.bbFunction
bbMode = io_vars.bbMode
bbPacketAddress = io_vars.bbPacketAddress
bbSource = io_vars.bbSource
bbChunk = io_vars.bbChunk
bbScanLeft = io_vars.bbScanLeft
bbMore = io_vars.bbMore
bbRegs = io_vars.bbRegs
bbCopy = io_vars.bbCopy
; Общий временный буфер: UI и физический A: не вложены друг в друга.
bbBuffer = BIOS_BANK_IO_BUFFER
    ORG_PAD MEM_MAIN_PUT32_TEMPLATE
    MODULE mainDword
    INCLUDE "../mem/put32-template.inc"
    ENDMODULE
bankBiosEnd:
    ASSERT $ <= BANK_BIOS_END
    END
