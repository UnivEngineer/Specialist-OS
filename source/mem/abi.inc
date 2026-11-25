; Раскладка 32-байтного аппаратного окна общей памяти FFC0h–FFDFh
; Полная карта IO/MEM и её адресные константы — ../include/mxos.inc.
; Все 32 байта содержат данные текущей операции. COMMAND размещает здесь
; регистровый кадр, TEXT и COPY — очередную порцию. Внутренний ABI.
; FAR: FAR-код, младший и старший байты смещения;
; номер банка: 0=main, 1..16=банки ДОЗУ.

BANK_WINDOW           = 0FFC0h
BANK_TEXT_BYTES       = 32
BANK_TEXT_STACK_MIN   = 29
MEM_TEXT_RAW_Z        = 1
MEM_TEXT_RAW_LIM      = 2
MEM_TEXT_LIM          = 3
MEM_TEXT_BATCH        = 7
MEM_TEXT_RAW_PSEUDO   = 8
MEM_WORD_RETURN       = 0F32Bh
MEM_TOP_MAPPED        = 0F4D7h
;------------------------------------------------------------------------------
; MXOS
; Выделенный вход курсора в копиях MEM. Чтение возвращает HL, запись принимает HL.
; AF, BC, DE и SP сохраняются. После сброса очереди стек нужен только инициатору.
; LD слова сохраняет знак DEC A: FFh означает MAIN, 0..15 — обратный селектор.
;------------------------------------------------------------------------------
    MACRO MEM_CURSOR_GATE name, read_cursor
name:
    call bankQueueBefore
    push af
    ld a,(bankPage)
    dec a
    jp m,.mapped
    ld (BANK_IO_MAIN),a
.mapped:
    IF read_cursor
        ld hl,(bios_vars.cursorY)
    ELSE
        ld (bios_vars.cursorY),hl
    ENDIF
    jp memFixedWordReturn
    ENDM
BANK_SELECT_GATE  = 0FED0h
BANK_SELECT_MAIN  = BANK_SELECT_GATE
BANK_SELECT_RAM   = BANK_SELECT_GATE
BANK_PACKET       = BANK_WINDOW+8
BANK_PACKET_SIZE  = 20

; Перенос из текущей страницы. B = предел, C = первый нетерминатор,
; HL = источник, DE = окно. На выходе HL/DE за скопированным фрагментом,
; CF = 1 при остановке на байте < C. C = 0 отключает остановку.
    MACRO MEM_TEXT_FILL repeat, done
repeat:
    ld a,(hl)
    inc hl
    ld (de),a
    inc de
    cp c
    jp c,done
    dec b
    jp nz,repeat
    ENDM

; COPY до переноса данных: пары порт/селектор, счётчик и SP инициатора.
BANK_COPY_SETUP_SOURCE = BANK_WINDOW
BANK_COPY_SETUP_DEST   = BANK_WINDOW+2
BANK_COPY_SETUP_HOME   = BANK_WINDOW+4
BANK_COPY_SETUP_COUNT  = BANK_WINDOW+6
BANK_COPY_SETUP_SP     = BANK_WINDOW+8
BANK_FRAME_REGS   = BANK_PACKET
BANK_FRAME_SP     = BANK_PACKET+8
BANK_FRAME_PAGE   = BANK_PACKET+10
BANK_FRAME_OWNER  = BANK_PACKET+11
BANK_FRAME_HOME   = BANK_PACKET+12
BANK_FRAME_KIND   = BANK_PACKET+13
BANK_FRAME_TARGET = BANK_PACKET+14

; Формат кадра MX_REGS: восемь байт регистров передаются прямо в окне.
; KIND=0 для запроса, KIND=1 для возврата. BC передаёт номер операции.
BANK_FRAME_STATUS    = BANK_PACKET+15
BANK_FRAME_ENTRY     = BANK_PACKET+16

; Во время точного остатка CPM32 нужны максимум 31 байт данных.
BANK_COPY_TINY_COUNT    = BANK_WINDOW+31
BANK_LANDING            = 0FC00h
BANK_COMMAND_LANDING    = BANK_SELECT_GATE
BANK_SCALAR_KIND        = BANK_FRAME_KIND
BANK_SCALAR_RETURN_PAIR = BANK_WINDOW+28

; Входы полной банковой DOS после локальной таблицы JMP.
BANK_DOS_GATE        = DOS_BANK_CODE_ADDR + (dos_setShell-dos_dosBoot) + 3
DOS_FAST_ERROR_ENTRY = BANK_DOS_GATE+12

; Две пары слов в MAIN: вектор C8xxh и адрес возврата MEM.
; Номера 17h и 1Ah дают смещения 0Ch и 10h.
BIOS_FAST_SLOT_BIAS = 6
BIOS_FAST_SLOT_MASK = 01Ch
    ASSERT (BIOS_BANK_GATE_ADDR & 255) == 0

BANK_STACK_LOW    = 0F800h
BANK_STACK_TOP    = 0FC00h
BANK_MAX_DEPTH    = 8
BANK_ERROR_PAGE   = MEM_ERROR_BANK
BANK_ERROR_STACK  = MEM_ERROR_STACK
BANK_IO_MAIN      = 0FFFCh
BANK_IO_RAM       = 0FFFDh
BANK_ERROR_RANGE  = MEM_ERROR_RANGE

; COPY/CPM32: окно целиком содержит данные. Селекторы и счётчик
; продублированы в хвостах страниц; адреса экспортирует транспортная сборка.

; Локальные кадры стека, не поля общего аппаратного окна.
; Все слова little-endian. Смещения от SP сразу после создания кадра.
; Кадр вызывающей стороны: BANK_FRAME_SP указывает на его начало.
BANK_CALLER_SERVICE_SP = 0   ; Предыдущее значение локального bankServiceSP
BANK_CALLER_PC         = 2   ; Адрес возврата, записанный исходным CALL
BANK_CALLER_BYTES      = 4
; Кадр исполнителя: SP на входе в целевую процедуру.
BANK_CALLEE_RETURN     = 0  ; Адрес bankServiceReturn для обычного RET
BANK_CALLEE_CALLER_SP  = 2  ; Указатель на кадр вызывающей стороны
BANK_CALLEE_SOURCE     = 4  ; FAR-код источника; старший байт слова равен 0
BANK_CALLEE_OWNER      = 6  ; Прежний локальный bankOwner
BANK_CALLEE_HOME       = 7  ; Прежний локальный bankHome
BANK_CALLEE_BYTES      = 8

; Только внутренний физический перенос: 80h..8Fh в srcPage/dstPage
; выбирает ROM-банк 0..15 через FFFEh. Публичный FAR такого кода не допускает.
; Профиль используется после проверки запроса драйвером, не аллокатором.


; Метки 32-байтной локальной развёртки в полном профиле.
MEM_LOCAL_FORWARD      = 0F928h
MEM_FORWARD_BLOCK      = MEM_LOCAL_FORWARD+8+5
MEM_FORWARD_BLOCK_END  = MEM_FORWARD_BLOCK+16*5
MEM_FORWARD_RESTORE    = MEM_FORWARD_BLOCK_END+8
MEM_SCREEN_RETURN      = 0F58Fh
MEM_TEXT_RETURN        = 0F561h
