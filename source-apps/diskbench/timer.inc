; Локальный ВИ53: одинаковый путь независимо от страницы измерителя.
    INCLUDE "../../source/include/timing-pit.inc"
timer_init = pitTimerInit
timer_read = pitTimerRead
timing_print = puts
timing_printChar:
    ld c,a
    jp bench_printChar
    DEFINE TIMING_NUMBER_WIDTH 8
DECIMAL = inputBuffer
    DEFINE TIMING_PROBE_READY pitTimerWaitReady
    INCLUDE "../../source/include/timing.inc"
    UNDEFINE TIMING_PROBE_READY

timing_div_hl_de = bench_div_hl_de
    INCLUDE "../../source/include/timing-ms.inc"
