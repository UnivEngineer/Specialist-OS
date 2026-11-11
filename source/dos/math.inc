; Локальная арифметика эталонной DOS; банковая сборка включает common/math.inc.
    IFDEF DOS_REFERENCE_BUILD
    DEFINE MATH_DIV2
    DEFINE MATH_MUL
    DEFINE MATH_DIV
    INCLUDE "../include/math.inc"
    ENDIF
