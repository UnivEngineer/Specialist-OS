; Одноразовый ROM-драйвер старой 64-КиБ платы без защёлок адреса.
    INCLUDE "../include/mxos.inc"
IMAGE_FLAGS = IMAGE_FLAG_DRIVER
LOCAL_KIND = 0
    DEFINE ROM_LEGACY 1
    INCLUDE "driver-init.inc"
    END
