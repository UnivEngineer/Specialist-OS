; Автономный стенд физического обмена в банковой памяти.
; Не определяет размещение и входы штатного файлового ядра DOS.
    INCLUDE "../../include/mxos.inc"
    INCLUDE "../../mem/abi.inc"
    INCLUDE "../../mem/diskabi.inc"
    INCLUDE "../../../bin/banked/transport-imports.inc"
    ORG DOS_IO_START
    INCLUDE "diskio.inc"
    INCLUDE "../../dos/ramdisk.inc"
    INCLUDE "../../dos/romproxy.inc"
dosIoEnd:
    ASSERT $ <= DOS_IO_BUFFER
    END
