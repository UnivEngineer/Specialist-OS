; DISKBEN использует аппаратный таймер через опубликованные входы DOS.
timer_init = bios_timerInit
timer_read = bios_timerRead
timing_print = puts
timing_printChar:
    ld c,a
    jp bios_printChar
TIMING_NUMBER_WIDTH = 8
DECIMAL = inputBuffer
    INCLUDE "../../source/include/timing.inc"

timing_div_hl_de = bios_div_hl_de
    INCLUDE "../../source/include/timing-ms.inc"
