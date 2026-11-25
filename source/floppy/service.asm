; Самостоятельная служба ВГ93. Заголовок и таблица входят в тело установщика.
    INCLUDE "../include/mxos.inc"
    ORG 0
    DW serviceEnd-serviceStart, fdTransfer-serviceStart, (tableEnd-tableStart)/2
    DISP 0
    RELOCATE_START
serviceStart:
    INCLUDE "transfer.inc"
serviceEnd:
    RELOCATE_END
    ENT
tableStart:
    RELOCATE_TABLE serviceStart
tableEnd:
    END
