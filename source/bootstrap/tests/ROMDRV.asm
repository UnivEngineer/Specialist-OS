; Внешний 64-КБ ROM через ВВ55 доступен из страницы DOS непосредственно.
    INCLUDE "../../include/mxos.inc"
DISK_SECTORS = 256
    ORG DOS_BANK_DRIVER_ADDR+0400h
    INCLUDE "../../flash/rom-driver.inc"
romDriverEnd:
    ASSERT $ <= DOS_BANK_DRIVER_ADDR+0600h
    END
