;+---------------------------------------------------------------------------
; MXOS
; TAPE.COM - драйвер работы с магнитофоном, вынесенный из DOS.SYS
;
; 2022-02-02 SpaceEngineer
;----------------------------------------------------------------------------

    INCLUDE "../include/mxos.inc"

    ORG     0E600h

	INCLUDE "tapeInit.inc"
	INCLUDE "tapeWriteDelay.inc"
	INCLUDE "tapeRead.inc"
	INCLUDE "tapeReadDelay.inc"
	INCLUDE "tapeWrite.inc"
	INCLUDE "tapeLoadInt.inc"
	INCLUDE "tapeReadError.inc"
	INCLUDE "tapeSave.inc"
	INCLUDE "tapeWriteWord.inc"
	INCLUDE "tapeLoad.inc"
	INCLUDE "cmp_hl_de_2.inc"

    ; Проверка - TAPE.COM не должен вылезать за эти пределы
    ASSERT_DONT_FIT 0E800h

    END
