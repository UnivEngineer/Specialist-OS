; RAMDISK — одноразовая подготовка B: с передачей фиксированного пула DOS.
    INCLUDE "../include/mxos.inc"
    IFNDEF RAMDISK_LEGACY
RAMDISK_LEGACY = 0
    ENDIF
    IF RAMDISK_LEGACY
IMAGE_FLAGS = IMAGE_FLAG_MAIN
    INCLUDE "legacy.inc"
    ELSE
IMAGE_FLAGS = IMAGE_FLAG_DRIVER
    IFNDEF DRIVER_ORIGIN
DRIVER_ORIGIN = DOS_BANK_DRIVER_ADDR
    ENDIF
DRIVER_LOAD_ADDR = DRIVER_ORIGIN
    ORG DRIVER_LOAD_ADDR
    jp RamdiskLauncher
    INCLUDE "launcher.inc"
    INCLUDE "args.inc"
BankInstall:
    INCLUDE "native-install.inc"
    ENDIF
    END
