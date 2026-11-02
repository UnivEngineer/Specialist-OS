; Выбор реализации прежнего физического ABI, без дополнительного диспетчера.
    IF DISK_DRIVER_CHECKED
        INCLUDE "driverChecked.inc"
    ELSE
        INCLUDE "driver.inc"
    ENDIF
