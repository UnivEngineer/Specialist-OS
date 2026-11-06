; Изолированная сборка будущего физического слоя DOS в её странице.
; Файловое ядро основной сборки пока сюда не маршрутизируется.
    INCLUDE "../include/mxos.inc"
    INCLUDE "abi.inc"
    INCLUDE "diskabi.inc"
    INCLUDE "../../bin/banked/transport-imports.inc"
    ORG DOS_IO_START
    INCLUDE "diskio.inc"
    INCLUDE "ramdisk.inc"
    INCLUDE "romproxy.inc"
dosIoEnd:
    ASSERT $ <= DOS_IO_BUFFER
    END

