; Внутренняя карта IO.SYS основной памяти.
; Файл F000h-F724h (1829 байт) загружается непрерывно. Слоты физических
; драйверов расположены отдельно и не включены в файл IO.SYS.
; Адреса/размеры текущей сборки уточняет bin/System.layout.json.
;   F000-F23B - [572 B] MAINBIOS.asm: принимающий адаптер BIOS
;   F23C-F23F - [  4 B] Выравнивание
;   F240-F43A - [507 B] MAINIO.asm: физический мост A: <-> DOS
;   F43B-F4EE - [180 B] legacy-files.inc: исторические файловые JMP
;   F4EF-F5DA - [236 B] fileBody.inc: загрузчик тела файла
;   F5DB-F691 - [183 B] raw-local.inc: прямой RAW и контекст MEM
;   F692-F717 - [134 B] glyphCache.inc: управление границами кэша
;   F718-F724 - [ 13 B] glyphCache.inc: очистка IO_VARIABLES
; Общие публичные JMP устанавливаются bootstrap из MEM.SYS.
; LOCALBLOCK (F740h), FLASHSYS (F8C0h), H: (FA00h) не входят в IO.SYS.
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
bbSlowEntry:
    push af
    ld a,(bbBusy)
    or a
    jp nz,bbReentered
    xor a
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
    cp MX_GROUP_BIOS
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
bbFastFetchLoop:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,bbFastFetchLoop
    ld a,1
    ld (bbBusy),a
bbRegsReady:
    ld a,(bbMode)
    dec a
    jp z,bbInvoke
    ld hl,(bbRegs)
    ld (bbSource),hl
    ld a,(bbMode)
    cp 2
    jp z,bbString
    call bbFetchBuffer
    jp c,bbCopyError
    ld hl,bbBuffer
    ld a,(bbMode)
    cp 3
    jp z,bbDrawScan
    jp bbToolbarScan

; Скалярные вызовы: адрес экрана и координаты — значения, не NEAR-буфер.
bbInvoke:
    ld a,(bbFunction)
    ld c,a
    ld b,MX_GROUP_BIOS
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
    call bbFetchBuffer
    jp c,bbCopyError
    ld de,0
    ld hl,(bbChunk)
    ld b,h
    ld c,l
    ld hl,bbBuffer
bbStringScan:
    ld a,(hl)
    or a
    jp z,bbStringTerminated
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,bbStringScan
    ld a,1
bbStringTerminated:
    ld (bbMore),a
    ld (hl),0
    push de
    ld hl,bbBuffer
    call bios_printString
    pop de
    ld hl,(bbSource)
    add hl,de
    ld (bbSource),hl
    ld a,(bbMore)
    or a
    jp nz,bbString
    ld (bbRegs),hl
    jp bbFinish

; Список UI_DRAW: записи 5 или 6 байт, затем 0. Буфер ограничен 256 байтами.
; Нельзя искать конец как ASCIIZ: координаты и маски содержат нули.
bbDrawScan:
    call bbScanByte
    jp c,bbCopyError
    or a
    jp z,bbListReady
    and 0Fh
    cp 1
    ld b,4
    jp nz,bbDrawSkip
    inc b
bbDrawSkip:
    call bbScanByte
    jp c,bbCopyError
    dec b
    jp nz,bbDrawSkip
    jp bbDrawScan

; Тулбар: [ширина/инверсия, ASCIIZ]...0. Не допускаем незавершённое поле.
bbToolbarScan:
    call bbScanByte
    jp c,bbCopyError
    or a
    jp z,bbListReady
    and 07Fh
    dec a
    cp 4
    jp nc,bbCopyError
bbToolbarText:
    call bbScanByte
    jp c,bbCopyError
    or a
    jp nz,bbToolbarText
    jp bbToolbarScan
bbListReady:
    ; uiDraw читает ещё четыре байта даже после терминатора.
    ; Нули после списка исключают чтение служебного состояния адаптера.
    xor a
    ld b,4
bbListPad:
    ld (hl),a
    inc hl
    dec b
    jp nz,bbListPad
    ld hl,bbBuffer
    ld (bbRegs),hl
    jp bbInvoke
bbScanByte:
    ld a,(bbScanLeft+1)
    or a
    jp nz,bbScanAvailable
    ld a,(bbScanLeft)
    or a
    scf
    ret z
bbScanAvailable:
    push hl
    ld hl,(bbScanLeft)
    dec hl
    ld (bbScanLeft),hl
    pop hl
    ld a,(hl)
    inc hl
    or a
    ret

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
bbFetchBuffer:
    ld hl,(bbSource)
    ld a,h
    cp BANK_CLIENT_LIMIT/256
    ccf
    ret c
    ld bc,BANK_BIOS_BUFFER_SIZE
    ; Последние байты перед хвостом читаются без выхода за границу.
    cp BANK_CLIENT_LIMIT/256-1
    jp nz,bbChunkReady
    ld a,l
    or a
    jp z,bbChunkReady
    cpl
    inc a
    ld c,a
    ld b,0
bbChunkReady:
    push hl
    ld h,b
    ld l,c
    ld (bbChunk),hl
    ld (bbScanLeft),hl
    pop hl
    ld de,bbBuffer
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
; 2 — ASCIIZ; 3 — список геометрии; 4 — список тулбара.
; Дисковые функции, memcpy/strcmp, установка драйверов/хуков и memtop
; нельзя исполнять над NEAR-адресами клиента в контексте main.
bbModes:
    DB 1,0,1,0,0,1,1,2,1,1,1,0,0,0,1,0
    DB 0,1,0,0,1,1,0,1,0,0,1,1,1,1,1,0
    DB 1,1,3,1,4,1,0,0,0,0,0,0,0
    DB 0 ; FILEBODY работает с NEAR основной памяти, как загрузчик RUN.
    DB 1 ; GLYPHCACHE передаёт границы как значения, не NEAR-буфер.
bbModesEnd:
    ASSERT bbModesEnd-bbModes == MX_BIOS_GLYPHCACHE-0100h+1
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
; Только строки и списки UI; физический обмен A: не использует этот буфер.
bbBuffer = BIOS_BANK_IO_BUFFER
bankBiosEnd:
    ASSERT $ <= BANK_BIOS_END
    END
