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
DRIVER_ORIGIN = 0
    ENDIF
    ORG DRIVER_ORIGIN
    RELOCATE_START
driverBody:
    jp RamdiskLauncher
    INCLUDE "launcher.inc"
    INCLUDE "args.inc"
BankInstall:
    INCLUDE "native-install.inc"
    RELOCATE_END
IMAGE_BODY_SIZE  = $-driverBody
IMAGE_MEM_BEGIN  = 0
IMAGE_MEM_END    = IMAGE_BODY_SIZE
IMAGE_BSS_SIZE   = 0
IMAGE_RELOC_KIND = 0
IMAGE_ALIGN_LOG2 = 0
    RELOCATE_TABLE driverBody
    ENDIF
    END
