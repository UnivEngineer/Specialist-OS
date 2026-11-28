; MAIN использует локальный BIOS; EXEC DLL — только публичный MXOS_CALL_REGS.
bench_input:
    MXOS_CALL_REGS MXOS_BIOS_INPUT
    ret
bench_printDecWord:
    MXOS_CALL_REGS MXOS_BIOS_PRINT_DEC_WORD
    ret
bench_upperCase:
    MXOS_CALL_REGS MXOS_BIOS_UPPERCASE
    ret
    IFDEF DISKBEN_BANK
bench_getch:
    MXOS_CALL_REGS MXOS_BIOS_GETCH
    ret
bench_getMemTop:
    call mx_getMemTop
    ret
bench_printChar:
    call mx_printChar
    ret
bench_printHexWord:
    call mx_printHexWord
    ret
bench_printString:
    MXOS_CALL_REGS MXOS_BIOS_PRINT_STRING
    ret
bench_cmp_hl_de:
    INCLUDE "../../source/common/cmp_hl_de.inc"
    ELSE
bench_getch        = mon2_getchOld
bench_getMemTop    = mx_getMemTop
bench_printChar    = mon2_printCharOld
bench_printHexWord = mx_printHexWord
bench_printString  = mon2_printStringOld
bench_cmp_hl_de    = mon2_cmp_hl_de
    ENDIF

; Арифметика всегда локальна, включая EXEC DLL: MXOS_CALL_REGS здесь только замедляет.
    MODULE benchMath
    DEFINE MATH_MUL
    DEFINE MATH_DIV2
    DEFINE MATH_DIV
    INCLUDE "../../sdk/asm/lib/math.inc"
    ENDMODULE
bench_mul_hl_de = benchMath.mul_hl_de
bench_div_hl_2  = benchMath.div_hl_2
bench_div_hl_de = benchMath.div_hl_de
