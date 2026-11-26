; MXOS
; Системные определения, профиль сборки и размещение.

    IFNDEF MXOS_SYSTEM_INC
    DEFINE MXOS_SYSTEM_INC

    INCLUDE "../../sdk/asm/mxos.inc"
    INCLUDE "../../sdk/asm/assembly.inc"
    INCLUDE "../../sdk/asm/drivers/disk.inc"
    INCLUDE "../../sdk/asm/formats/fat.inc"
    INCLUDE "../../sdk/asm/formats/font.inc"
    INCLUDE "../../sdk/asm/hardware/spec_mx.inc"
    INCLUDE "../dos/internal.inc"
    INCLUDE "system-layout.inc"

    ENDIF
