; Пересчёт и вывод измерений модуля.
timer_read   = io_pitRead
timing_print = puts
timing_printChar:
    ld c,a
    jp bench_printChar
    DEFINE TIMING_NUMBER_WIDTH 8
DECIMAL = inputBuffer
    INCLUDE "../../sdk/asm/lib/timing.inc"

timing_div_hl_de = bench_div_hl_de
    INCLUDE "../../sdk/asm/lib/timing-ms.inc"
