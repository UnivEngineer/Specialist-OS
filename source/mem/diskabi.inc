; Внутренний ABI стенда банковых драйверов, ещё не публичный системный вызов.
; DOS в физической странице 0 (логическая FAR-страница 1).
DOS_IO_PAGE = DOS_BANK_PAGE
DOS_IO_START = 04000h ; Только автономный DISKIO; адрес штатной DOS задаётся отдельно.
; Буферы только отдельного DISKIO-стенда; полный KERNEL использует свою карту.
DOS_IO_BUFFER = 08800h
DOS_IO_VERIFY = 08A00h
DOS_IO_BUFFER_SIZE = 512
DOS_DRIVER_START = DOS_BANK_DRIVER_ADDR
DOS_DRIVER_END = DOS_BANK_DRIVER_END
DOS_DRIVER_SLOT_SIZE = 256
DOS_IO_CLIENT_END = 0F800h
; Аппаратный обмен системного ROM/flash размещён в резидентном BIOS.
BIOS_BANK_IO_START = BIOS_BANK_IO_ADDR

; Запрос: drive, operation (1/2/5), block:word, bufferPage, bufferOffset:word.
; Размер обмена определяется драйвером (128/256/512), не вызывающим.
DOS_IO_REQUEST_SIZE = 7

