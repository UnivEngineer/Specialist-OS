;+---------------------------------------------------------------------------
; MXOS
; TAPE.COM - самостоятельная утилита работы с магнитофоном.
; Загружается вместо NC.COM и использует локальный низкоуровневый драйвер.
;----------------------------------------------------------------------------

    INCLUDE "../include/mxos.inc"
IMAGE_FLAGS = IMAGE_FLAG_MAIN

    ORG     SYSTEM_PROGRAM_ADDR

    INCLUDE "tapeUi.inc"

    ; Фиксированная область низкоуровневых процедур нужна эмулятору:
    ; адреса этих диапазонов заданы в SpecialistMX2_My_MXOS.cfg.
    ORG_PAD SYSTEM_PROGRAM_ADDR + 0500h

    INCLUDE "tapeWriteDelay.inc"
    INCLUDE "tapeRead.inc"
    INCLUDE "tapeReadDelay.inc"
    INCLUDE "tapeWrite.inc"
    INCLUDE "tapeReadError.inc"
    INCLUDE "tapeWriteWord.inc"

    ; TAPE.COM не должен попасть в кэш FAT.
    ASSERT_DONT_FIT 0FB00h

    END