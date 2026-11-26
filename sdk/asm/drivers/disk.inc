; MXOS
; Пакет установки физического драйвера из банка DOS.

    IFNDEF MXOS_DRIVER_DISK_INC
    DEFINE MXOS_DRIVER_DISK_INC

    INCLUDE "../abi/dos.inc"
    INCLUDE "../formats/relocations.inc"

; Установка переносимой службы из инициализатора COM в странице DOS.
; H: использует локальный слот, D: оставляет одну службу в странице DOS.
; Все указатели относятся к загруженному инициализатору; DOS копирует код и строки до его освобождения.
DRIVER_INSTALL_VERSION = 1
DRIVER_FLOPPY_RELOC    = 2  ; image указывает на описание перемещаемой службы

DRIVER_KIND_FLOPPY = 3

    STRUCT DRIVER_INSTALL
version             DB 1
drive               DB 7
kind                DB 0       ; 0 ROM, 1 FLASH, 2 FLASHFST, 3 FLOPPY
blockShift          DB 8
blocks              DW 0       ; Физическая ёмкость, 0 для службы без носителя
allocationStep      DW 0
capabilities        DB 0
technology          DB 0
image               DW 0       ; Ссылка на тело H: или пакет службы ВГ93
imageSize           DW 0
text                DW 0       ; две ASCIIZ-строки, вместе не более 64 байт
    ENDS

    ASSERT DRIVER_INSTALL == 16

; ABI 1 драйвера тома. Аппаратный ABI bios_diskDriver остаётся отдельным.
; Таблица начинается с DB "FS",1,15; далее 15 адресов DW в этом порядке.
; Файловые операции пока сохраняют исторические контракты DOS (8.3, точный размер N).
; Это ещё не независимый байтовый поток и не готовый драйвер RAMFOS.

    ENDIF
