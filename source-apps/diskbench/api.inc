; MAIN использует локальный BIOS; EXEC DLL — только публичный MXOS_CALL.
    IFDEF DISKBEN_BANK
bench_getch:
    MXOS_CALL MXOS_BIOS_GETCH
    ret
bench_getMemTop:
    MXOS_CALL MXOS_BIOS_GET_MEM_TOP
    ret
bench_input:
    MXOS_CALL MXOS_BIOS_INPUT
    ret
bench_printChar:
    MXOS_CALL MXOS_BIOS_PRINT_CHAR
    ret
bench_printDecWord:
    MXOS_CALL MXOS_BIOS_PRINT_DEC_WORD
    ret
bench_printHexWord:
    MXOS_CALL MXOS_BIOS_PRINT_HEX_WORD
    ret
bench_printString:
    MXOS_CALL MXOS_BIOS_PRINT_STRING
    ret
bench_upperCase:
    MXOS_CALL MXOS_BIOS_UPPERCASE
    ret
bench_cmp_hl_de:
    INCLUDE "../../source/common/cmp_hl_de.inc"
    ELSE
bench_getch = bios_getch
bench_getMemTop = bios_getMemTop
bench_input = bios_input
bench_printChar = bios_printChar
bench_printDecWord = bios_printDecWord
bench_printHexWord = bios_printHexWord
bench_printString = bios_printString
bench_upperCase = bios_upperCase
bench_cmp_hl_de = bios_cmp_hl_de
    ENDIF

; Арифметика всегда локальна, включая EXEC DLL: MXOS_CALL здесь только замедляет.
    MODULE benchMath
    DEFINE MATH_MUL
    DEFINE MATH_DIV2
    DEFINE MATH_DIV
    INCLUDE "../../source/include/math.inc"
    ENDMODULE
bench_mul_hl_de = benchMath.mul_hl_de
bench_div_hl_2 = benchMath.div_hl_2
bench_div_hl_de = benchMath.div_hl_de
