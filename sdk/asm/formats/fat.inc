; MXOS
; Параметры исторического формата тома и метки FAT.

    IFNDEF MXOS_FORMAT_FAT_INC
    DEFINE MXOS_FORMAT_FAT_INC

    INCLUDE "directory.inc"

FAT_ITEM_SIZE = 2  ; размер записи в таблице FAT (байт)

SECTOR_SIZE  = 00100h  ; размер сектора 256 байт
CLUSTER_SIZE = 00100h  ; размер кластера 256 байт

; Специальные коды кластеров FAT
FAT12_BAD          = 00FF7h  ; плохой кластер
FAT16_BAD          = 0FFF7h
FAT12_EOC          = 00FFFh  ; последний кластер файла
FAT16_EOC          = 0FFFFh
FAT12_RES          = 00FFFh  ; зарезервированный кластер
FAT16_RESERVED_MIN = 0FFF0h
FAT16_EOC_MIN      = 0FFF8h
FAT16_RES          = 0FFF0h

    ENDIF
