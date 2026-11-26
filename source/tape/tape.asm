;+---------------------------------------------------------------------------
; MXOS
; TAPE.COM - самостоятельная утилита работы с магнитофоном.
; Загружается вместо NC.COM и использует локальный низкоуровневый драйвер.
;----------------------------------------------------------------------------

    INCLUDE "../common/system.inc"
IMAGE_FLAGS = IMAGE_FLAG_MAIN

    ORG     SYSTEM_PROGRAM_ADDR

    INCLUDE "tapeUi.inc"

    ; Сохранить фиксированное размещение низкоуровневых процедур.
    ORG_PAD SYSTEM_PROGRAM_ADDR + 0500h

    INCLUDE "tapeWriteDelay.inc"
    INCLUDE "tapeRead.inc"
    INCLUDE "tapeReadDelay.inc"
    INCLUDE "tapeWrite.inc"
    INCLUDE "tapeReadError.inc"
    INCLUDE "tapeWriteWord.inc"

    ; TAPE.COM не должен попасть в кэш FAT.
    ASSERT_DONT_FIT 0FB00h

    INCLUDE "../../sdk/asm/lib/file-io.inc"
    END