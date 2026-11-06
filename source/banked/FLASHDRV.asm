; Драйвер H: в странице DOS. Та же аппаратная реализация, что FLASH.COM.
; Пока фиксированная сборка слотов: релокируемый формат — отдельный этап.
    INCLUDE "../include/mxos.inc"
    INCLUDE "diskabi.inc"
    IFNDEF FLASH_SLOT
FLASH_SLOT = DOS_DRIVER_START
    ENDIF
FLASH_BANKED_DRIVER = 1
LATCH_0 = 00Ah
LATCH_1 = 00Bh
WRITE_0 = 00Ch
WRITE_1 = 00Dh
MASK_STANDBY = 0C0h
MASK_READ = 040h
PPI_MODE_WRITE = 080h
PPI_MODE_READ = 090h
    ORG FLASH_SLOT
    INCLUDE "../flash/flash-driver.inc"

    INCLUDE "flash-init.inc"
flashDosEnd:
    ASSERT $ <= DOS_DRIVER_END
    ASSERT (FLASH_SLOT & 0FFh) == 0
    END
