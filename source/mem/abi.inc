; Раскладка 32-байтного аппаратного окна общей памяти FFC0h–FFDFh
; для стандартного протокола команд. Внутренний ABI, пока не публичный.
; memcpy_far использует другой профиль ниже; пакетная печать может иметь
; собственный профиль. 20 байт стандартного пакета НЕ являются буфером строки.
; FAR: FAR-код, младший и старший байты смещения;
; номер банка: 0=main, 1..16=банки ДОЗУ.
BANK_WINDOW       = 0FFC0h
BANK_SELECT_MAIN  = BANK_WINDOW
BANK_SELECT_RAM   = BANK_WINDOW
BANK_PACKET       = BANK_WINDOW+8
BANK_PACKET_SIZE  = 20
BANK_FRAME_REGS   = BANK_PACKET
BANK_FRAME_SP     = BANK_PACKET+8
BANK_FRAME_PAGE   = BANK_PACKET+10
BANK_FRAME_OWNER  = BANK_PACKET+11
BANK_FRAME_HOME   = BANK_PACKET+12
BANK_FRAME_KIND   = BANK_PACKET+13
BANK_FRAME_TARGET = BANK_PACKET+14

; Профиль вызова MX_REGS: восемь байт регистров передаются прямо в окне.
; KIND=2, ENTRY/OPERATION не занимают регистры вызываемой функции.
BANK_FRAME_STATUS = BANK_PACKET+15
BANK_FRAME_ENTRY = BANK_PACKET+16
BANK_FRAME_OPERATION = BANK_PACKET+18
    ASSERT BANK_FRAME_OPERATION+2 == BANK_WINDOW+28
; Последний байт физически общий и не пересекается с командами/CPM16.
BANK_PROFILE = BANK_WINDOW+31
BANK_PROFILE_COMMAND = 0
BANK_PROFILE_COPY = 1
BANK_PROFILE_TEXT = 2
BANK_PROFILE_LOCAL = 3
BANK_PROFILE_COUNT = 4
BANK_LANDING      = 0FC00h

; Временный вход полной банковой DOS после локальной таблицы JMP.
BANK_DOS_GATE     = 0015Dh
BANK_STACK_LOW    = 0F800h
BANK_STACK_TOP    = 0FC00h
BANK_MAX_DEPTH    = 8
BANK_ERROR_PAGE   = MEM_ERROR_BANK
BANK_ERROR_STACK  = MEM_ERROR_STACK
BANK_IO_MAIN      = 0FFFCh
BANK_IO_RAM       = 0FFFDh
BANK_ERROR_RANGE  = MEM_ERROR_RANGE

; Профиль копировщика CPM16: первые 16 байт — данные, затем параметры переноса.
; Пары селекторов: младший байт — FCh/FDh, старший — физическая страница.
; SP и селектор возврата принадлежат инициатору, а не источнику.
BANK_COPY_SOURCE     = BANK_WINDOW+16
BANK_COPY_DEST       = BANK_WINDOW+18
BANK_COPY_SP         = BANK_WINDOW+20
BANK_COPY_SRC_SELECT = BANK_WINDOW+22
BANK_COPY_DST_SELECT = BANK_WINDOW+24
BANK_COPY_RETURN     = BANK_WINDOW+26
BANK_COPY_REST       = BANK_WINDOW+28

; Локальные кадры стека, не поля общего аппаратного окна.
; Все слова little-endian. Смещения от SP сразу после создания кадра.
; Кадр вызывающей стороны: BANK_FRAME_SP указывает на его начало.
BANK_CALLER_SERVICE_SP = 0  ; Предыдущее значение локального bankServiceSP
BANK_CALLER_HL         = 2  ; Сохранённый HL
BANK_CALLER_DE         = 4  ; Сохранённый DE
BANK_CALLER_BC         = 6  ; Сохранённый BC
BANK_CALLER_AF         = 8  ; Сохранённый AF: F по +8, A по +9
BANK_CALLER_PC         = 10 ; Адрес возврата, записанный исходным CALL
BANK_CALLER_BYTES      = 12
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

