; Автономный стенд физического обмена в банковой памяти.
; Не определяет размещение и входы штатного файлового ядра DOS.
    INCLUDE "../../common/system.inc"
    INCLUDE "../../mem/abi.inc"
    INCLUDE "../../mem/diskabi.inc"
    INCLUDE "../../../bin/banked/transport-imports.inc"
    ORG DOS_IO_START
    INCLUDE "diskio.inc"
    INCLUDE "../../dos/ramdisk.inc"
    INCLUDE "../../dos/romproxy.inc"
; Private working data of the stand-alone physical I/O fixture.
dosRamCopy: DB 0
    DW 0
    DB 0
    DW 0,256
dosSystemRequest: DS 5,0
dosIoEnd:
    ASSERT $ <= DOS_IO_BUFFER
    END
