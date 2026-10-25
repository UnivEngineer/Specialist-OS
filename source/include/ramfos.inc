;----------------------------------------------------------------------------
; Формат загружаемого ядра RAMFOS. Это не таблица вызовов MXOS.
; В файле подряд идут C000h-D2FFh и F800h-FFDFh, без промежутка D300h-F7FFh.
;----------------------------------------------------------------------------
RAMFOS_CORE_ADDR  = 0C000h
RAMFOS_CORE_SIZE  = 01300h
RAMFOS_HIGH_ADDR  = 0F800h
RAMFOS_HIGH_SIZE  = 007E0h
RAMFOS_IMAGE_SIZE = RAMFOS_CORE_SIZE + RAMFOS_HIGH_SIZE
RAMFOS_START      = 0F85Ah ; HL = адрес запуска, DE = строка аргументов
RAMFOS_TEMP       = 0DC00h ; временно заняты драйверы и шрифт MXOS
RAMFOS_BRIDGE     = 08C00h ; переходник до завершения замены ядра
RAMFOS_ARGS       = 08D00h ; 128 байт аргументов при запуске
RAMFOS_CONTEXT    = 08D80h ; адрес запуска и длина программы
RAMFOS_BRIDGE_SP  = 08E00h ; временный стек переходника

    ASSERT RAMFOS_TEMP + ((RAMFOS_IMAGE_SIZE+255)/256)*256 <= RAMFOS_HIGH_ADDR
