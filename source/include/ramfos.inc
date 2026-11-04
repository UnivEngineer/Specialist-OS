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
RAMFOS_TEMP       = 09000h ; скрытый экран: драйверы F600h–F9FFh ещё нужны для чтения
RAMFOS_BRIDGE     = 08C00h ; переходник MX/STD, вне загружаемой программы
RAMFOS_ARGS       = 08D00h ; аргументы всех режимов, доступны в STD
RAMFOS_CONTEXT    = 08D80h ; адрес запуска и длина программы
RAMFOS_VIDEO_BRIDGE = 0AB00h ; код подмены ядра, после 6880 байт образа
RAMFOS_VIDEO_SP = 0BF00h ; временный стек до передачи новому ядру
RAMFOS_BRIDGE_SP  = 08F00h ; Ниже стека вызывающей стороны: копирование не затирает её RET ; временный стек переходника

    ASSERT RAMFOS_TEMP + ((RAMFOS_IMAGE_SIZE+255)/256)*256 <= RAMFOS_CORE_ADDR
