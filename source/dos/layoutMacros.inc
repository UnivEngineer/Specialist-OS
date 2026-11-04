; Фиксированный вход: только один трёхбайтный JP и неизменный адрес ABI.
    MACRO DOS_ABI_GATE address, target
        ASSERT $ == address
        jp target
        ASSERT $ == address + 3
        ASSERT target != address
        ASSERT (target >= DOS_CODE_START && target < DOS_CODE_END_MAX) || (target >= 0C000h && target < DOS_END_MAX)
    ENDM

; Обход фиксированного входа между двумя частями реализации.
; JP сохраняет регистры, флаги и SP. Место под него учитывает размещатель.
    MACRO DOS_ABI_BRIDGE target, limit
        ASSERT $ + 3 <= limit
        jp target
        ASSERT $ <= limit
        ASSERT (target >= DOS_CODE_START && target < DOS_CODE_END_MAX) || (target >= 0C000h && target < DOS_END_MAX)
    ENDM
