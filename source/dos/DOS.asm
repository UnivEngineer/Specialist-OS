; Файловое ядро и CLI в физической странице ДОЗУ 0.
; 0000h–00FFh оставлены под будущие векторы; образ содержит только код/данные.
    DEFINE FAT_CACHE_ADDR DOS_BANK_CACHE_ADDR
    DEFINE BANKED_DOS_BUILD 1
    INCLUDE "../include/mxos.inc"
    INCLUDE "../mem/diskabi.inc"
    INCLUDE "../mem/abi.inc"
    INCLUDE "../../bin/banked/transport-imports.inc"
    INCLUDE "../../bin/banked/flash-imports.inc"
; Локальные данные не должны пересекать общий хвост и стек F400h–F7FFh.
    DEFINE CMD_LINE DOS_BANK_CMD_ADDR
BANK_KERNEL_CACHE = DOS_BANK_CACHE_ADDR
bankKernelCommand = DOS_BANK_CMD_ADDR
; BIOS вызывается через F806h; функции над NEAR-памятью остаются локальными.
    DEFINE bios_printChar bankDosPrintChar
    DEFINE bios_printString bankDosPrintString
    DEFINE bios_printDecWord bankDosPrintDecWord
    DEFINE bios_getch bankDosGetch
    DEFINE bios_blockIO blockIO
    ORG DOS_CODE_START
dosCoreStart:
    INCLUDE "../../bin/banked/kernel-layout.inc"
    INCLUDE "kernel-drivers.inc"
    INCLUDE "kernel-bios.inc"
    INCLUDE "kernel-dispatch.inc"
    INCLUDE "kernel-stream.inc"
    INCLUDE "kernel-path.inc"
    INCLUDE "kernel-list.inc"
    INCLUDE "kernel-meta.inc"
    INCLUDE "kernel-filestream.inc"
    INCLUDE "kernel-observer.inc"
    INCLUDE "kernel-image.inc"
    INCLUDE "kernel-copy.inc"
dosCoreEnd:
dosExtensionStart:
    INCLUDE "kernel-exec.inc"
    INCLUDE "kernel-system.inc"
    INCLUDE "kernel-localio.inc"
    INCLUDE "kernel-registry.inc"
kernelEnd:
dosImageEnd = kernelEnd
    ASSERT $ <= DOS_CODE_END_MAX
    ASSERT $ <= DOS_BANK_DATA_ADDR
    ASSERT FAT_CACHE_ADDR+FAT_CACHE_SIZE <= CMD_LINE
    END
