; Файловое ядро и CLI в физической странице ДОЗУ 0.
; 0000h–00FFh оставлены под будущие векторы; образ содержит только код/данные.
    DEFINE FAT_CACHE_ADDR DOS_BANK_CACHE_ADDR
    DEFINE FAT_CACHE_TABLE_ADDR DOS_BANK_CACHE_TABLE
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
    INCLUDE "kernel-batch.inc"
    INCLUDE "kernel-ranges.inc"
    INCLUDE "kernel-list.inc"
    INCLUDE "kernel-meta.inc"
    INCLUDE "kernel-observer.inc"
    INCLUDE "kernel-image.inc"
    INCLUDE "kernel-copy.inc"
    INCLUDE "kernel-fileops.inc"
    INCLUDE "kernel-enumerate.inc"
dosCoreEnd:
dosExtensionStart:
    INCLUDE "kernel-exec.inc"
    INCLUDE "kernel-system.inc"
    INCLUDE "font-state.inc"
    INCLUDE "kernel-localio.inc"
    INCLUDE "kernel-registry.inc"
    INCLUDE "memory.inc"
    INCLUDE "modules.inc"
    INCLUDE "arena.inc"
kernelEnd:
dosImageEnd = kernelEnd
    ASSERT $ <= DOS_CODE_END_MAX
    ASSERT DOS_BANK_DATA_ADDR-kernelEnd <= 1024
    ASSERT $ <= DOS_BANK_OBJECT_ADDR
    ASSERT FAT_CACHE_ADDR+FAT_CACHE_SIZE <= DOS_BANK_STACK_BOTTOM
    END
