; Раскладка 32-байтного аппаратного окна общей памяти FFC0h–FFDFh
; для стандартного протокола команд. Внутренний ABI, пока не публичный.
; memcpy_far использует другой профиль ниже; пакетная печать может иметь
; собственный профиль. 20 байт стандартного пакета НЕ являются буфером строки.
; FAR: байт страницы, младший и старший байты смещения; 0=main, 1..16=ДОЗУ.
BANK_WINDOW       = 0FFC0h
BANK_SELECT_MAIN  = BANK_WINDOW
BANK_SELECT_RAM   = BANK_WINDOW+6
BANK_PACKET       = BANK_WINDOW+12
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
    ASSERT BANK_FRAME_OPERATION+2 == BANK_WINDOW+32
BANK_LANDING      = 0FC00h
; Временный вход полной банковой DOS после локальной таблицы JMP.
BANK_DOS_GATE     = 0405Dh
BANK_STACK_LOW    = 0F800h
BANK_STACK_TOP    = 0FC00h
BANK_MAX_DEPTH    = 8
BANK_ERROR_PAGE   = 0E0h
BANK_ERROR_STACK  = 0E1h
BANK_IO_MAIN      = 0FFFCh
BANK_IO_RAM       = 0FFFDh
BANK_ERROR_RANGE  = 0E2h

; Профиль CPM16: первые 16 байт — данные, затем параметры переноса.
; Пары селекторов: младший байт — FCh/FDh, старший — физическая страница.
; SP и селектор возврата принадлежат инициатору, а не источнику.
BANK_COPY_SOURCE     = BANK_WINDOW+16
BANK_COPY_DEST       = BANK_WINDOW+18
BANK_COPY_SP         = BANK_WINDOW+20
BANK_COPY_SRC_SELECT = BANK_WINDOW+22
BANK_COPY_DST_SELECT = BANK_WINDOW+24
BANK_COPY_RETURN     = BANK_WINDOW+26
BANK_COPY_REST       = BANK_WINDOW+28
