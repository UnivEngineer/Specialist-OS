; Внутреннее размещение адаптера BIOS на этапе интеграции.
; Публичные номера функций и MX_REGS определены только в mxos.inc.
; MAINBIOS не содержит копий рендера: вызывает установленный BIOS через F800h.
BANK_BIOS_GATE = BIOS_BANK_GATE_ADDR
BANK_BIOS_END = BIOS_BANK_GATE_END
BANK_BIOS_STACK_LOW = BIOS_BANK_STACK_BOTTOM
BANK_BIOS_STACK_TOP = BIOS_BANK_STACK_TOP
BANK_BIOS_BUFFER_SIZE = 256
BANK_CLIENT_LIMIT = 0F800h
