; MEM.SYS: плотный установочный образ общих служб исполняемых банков.
; Каноническая карта MAIN/DOS/EXEC/DATA и адреса IO/MEM — ../include/mxos.inc.
; Полный хвост F1C0h–FFBFh. Профиль DATA начинается по MEM_DATA_START.
; Сегменты файла и резерв хвоста различаются: слоты A:/H: устанавливаются отдельно.
; Заголовок MXMM, версия, число сегментов; записи {адрес, размер, смещение}.
; Банки DATA получают часть TRANSPORT от MEM_DATA_START до bankDataTailEnd.
    INCLUDE "../include/mxos.inc"
    INCLUDE "../../bin/banked/transport-imports.inc"
    ORG 0
    DB 'MXMM',1,4
    DW MEM_IMAGE_LOCAL_START,MEM_LOCAL_SIZE,MEM_LOCAL_OFFSET
    DW mx_callPacket,MEM_CLIENT_SIZE,MEM_CLIENT_OFFSET
    DW 0FC00h,MEM_TRANSPORT_SIZE,MEM_TRANSPORT_OFFSET
    DW 0FFC0h,MEM_WINDOW_SIZE,MEM_WINDOW_OFFSET
MEM_LOCAL_OFFSET:
    INCBIN "../../bin/banked/LOCALBLOCK.bin"
MEM_LOCAL_SIZE = $-MEM_LOCAL_OFFSET
MEM_CLIENT_OFFSET:
    INCBIN "../../bin/banked/CLIENT.bin"
MEM_CLIENT_SIZE = $-MEM_CLIENT_OFFSET
MEM_TRANSPORT_OFFSET:
    INCBIN "../../bin/banked/TRANSPORT.bin"
MEM_TRANSPORT_SIZE = $-MEM_TRANSPORT_OFFSET
MEM_DATA_OFFSET    = MEM_TRANSPORT_OFFSET+(MEM_DATA_START-0FC00h)
MEM_DATA_SIZE      = BANK_COPY_DATA_BYTES
MEM_WINDOW_OFFSET:
    INCBIN "../../bin/banked/WINDOW.bin"
MEM_WINDOW_SIZE = $-MEM_WINDOW_OFFSET
    ASSERT MEM_LOCAL_SIZE <= 06A0h
    ASSERT MEM_CLIENT_SIZE <= LOCAL_DRIVER_ADDR-mx_callPacket
    ASSERT BANK_MAIN_CLIENT_BYTES > 0 && BANK_MAIN_CLIENT_BYTES <= LOCAL_DRIVER_ADDR-mx_callPacket
    ASSERT BANK_MAIN_LOCAL_BYTES == SYSTEM_FLASH_ADDR-MEM_IMAGE_LOCAL_START
    ASSERT MEM_TRANSPORT_SIZE <= 03C0h
    ASSERT MEM_DATA_SIZE <= 0FFC0h-MEM_DATA_START
    ASSERT MEM_WINDOW_SIZE == 32
    END
