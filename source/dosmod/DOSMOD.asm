; Общие загружаемые службы MXOS. Таблица JMP — самостоятельный ABI модуля.
    INCLUDE "../include/mxos.inc"
    INCLUDE "../../bin/DOS.module.inc"
    ORG DOS_MODULE_ADDR
moduleStart:
    jp moduleReturn
    DB "DM",1
    DW DOS_MODULE_IMPORT_SUM
    DW moduleEnd-moduleStart
    DS 6,0
    ASSERT $ == DOS_MODULE_JUMPS
    jp uiDraw
    jp uiInverseRect
    jp uiToolbar
    jp uiCopyRect
    jp fileAssociation
    jp fileCopy
    jp fileHandle
moduleReturn:
    ret
    INCLUDE "../dos/ui/draw.inc"
    INCLUDE "../dos/ui/inverse.inc"
    INCLUDE "../dos/ui/toolbar.inc"
    INCLUDE "../dos/ui/copyRect.inc"
    INCLUDE "../dos/fileAssociation.inc"
    INCLUDE "../dos/fileCopy.inc"
    INCLUDE "../dos/fileHandle.inc"
moduleEnd:
    ASSERT $ <= DOS_MODULE_END
    END
