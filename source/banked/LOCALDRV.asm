; Исполняемая копия H: для слота клиента. Установка и выбор остаются в DOS.
    INCLUDE "../include/mxos.inc"
    DEFINE FLASH_BANKED_DRIVER 1
    DEFINE DRIVER_IO_ONLY 1
LATCH_0 = 00Ah
LATCH_1 = 00Bh
WRITE_0 = 00Ch
WRITE_1 = 00Dh
MASK_STANDBY = 0C0h
MASK_READ = 040h
PPI_MODE_WRITE = 080h
PPI_MODE_READ = 090h
CHIP_STATE_SIZE = 4
CHIP_STATE_COUNT = 4
    ORG LOCAL_DRIVER_ADDR
    IF LOCAL_KIND == 0
DISK_SECTORS = 256
        INCLUDE "../flash/rom-driver.inc"
    ELSE
        IF LOCAL_KIND == 1
            INCLUDE "../flash/flash-driver.inc"
        ELSE
            INCLUDE "../flash/flash-fast-driver.inc"
        ENDIF
    ENDIF
localDriverEnd:
    ASSERT $ <= LOCAL_DRIVER_END
    END
