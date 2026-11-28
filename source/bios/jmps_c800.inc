; Совместимая таблица MAIN. При старте загружается профиль RAMFOS.
biosCompatVectors:
    DEFINE BIOS_RAMFOS_JUMPS
    INCLUDE "ramfos-vectors.inc"
    UNDEFINE BIOS_RAMFOS_JUMPS
biosCompatVectorsEnd:
