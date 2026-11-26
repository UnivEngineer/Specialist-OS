; MXOS
; Операции MXOS_DOS_TIMER.

    IFNDEF MXOS_ABI_TIMER_INC
    DEFINE MXOS_ABI_TIMER_INC

    INCLUDE "calls.inc"

; RAW/CLOCK захватывают таймер, RELEASE доступен владельцу.
TIMER_RAW         = 0
TIMER_CLOCK       = 1
TIMER_RELEASE     = 2
TIMER_QUERY       = 3       ; A=0 свободен, 1 наш, 2 чужой

; Локальные входы IO.SYS, одинаковые в MAIN, DOS и EXEC.
io_pitRead        = 0F1B1h  ; HL = счётчик 2, A = L. BC, DE и флаги сохранены.
io_pitGetPort     = 0F1BBh  ; CF=0: HL = адрес CH0. CF=1: L = порт CH0, H=0.

; Смещения регистров относительно CH0 для обоих способов адресации.
PIT_CH0           = 0
PIT_CH1           = 1
PIT_CH2           = 2
PIT_CONTROL       = 3

    ENDIF
