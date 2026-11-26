; MXOS
; Коды возврата DOS в A. При успехе CF=0 и A=0.

    IFNDEF MXOS_ABI_DOS_ERRORS_INC
    DEFINE MXOS_ABI_DOS_ERRORS_INC

DOS_ERR_IO              = 1   ; Ошибка чтения, записи или сохранения кэша.
DOS_ERR_NOT_FOUND       = 2   ; Файл не найден.
DOS_ERR_EXISTS          = 3   ; Файл назначения уже существует.
DOS_ERR_FULL            = 4   ; На томе не хватает свободных кластеров.
DOS_ERR_DIR_FULL        = 5   ; Нет свободной записи в каталоге.
DOS_ERR_ACCESS          = 6   ; Операция запрещена атрибутами или режимом открытия.
DOS_ERR_ARGUMENT        = 7   ; Недопустимый параметр, адрес, размер или режим.
DOS_ERR_CORRUPT         = 8   ; Повреждена структура тома или цепочка кластеров.
DOS_ERR_NO_DRIVER       = 9   ; Для накопителя не установлен драйвер.
DOS_ERR_UNSUPPORTED     = 10  ; Формат тома или операция не поддерживаются.
DOS_ERR_CANCELLED       = 12  ; Cooperative cancellation at a safe boundary.
DOS_ERR_BAD_IMAGE       = 11  ; Неверный заголовок или контрольная сумма программы.
DOS_ERR_WRITE_PROTECTED = 13  ; Драйвер тома не поддерживает запись.
DOS_ERR_NO_MEDIA        = 14  ; Носитель отсутствует или его формат не распознан.

    ENDIF
