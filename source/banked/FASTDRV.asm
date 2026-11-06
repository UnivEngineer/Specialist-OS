; Пакетный драйвер H: с независимым программированием чипов.
    INCLUDE "../include/mxos.inc"
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
CHIP_STATE.busy = 0
CHIP_STATE.pageSelect = 1
CHIP_STATE.addrHi = 2
CHIP_STATE.data = 3
    ORG DOS_BANK_DRIVER_ADDR+0600h
    INCLUDE "../flash/flash-fast-driver.inc"
    INCLUDE "../dos/diskWriteChecked.inc"
    INCLUDE "flash-init.inc"
    ASSERT $ <= DOS_BANK_DRIVER_END
    END
