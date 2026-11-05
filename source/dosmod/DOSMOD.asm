; Общие загружаемые службы MXOS. Таблица JMP — самостоятельный ABI модуля.
    INCLUDE "../include/mxos.inc"
    INCLUDE "../../bin/DOS.module.inc"
    ORG DOS_MODULE_LOAD_ADDR
moduleStart:
    INCLUDE "../cli/command.inc"
    INCLUDE "../cli/directories.inc"
    INCLUDE "../cli/attrib.inc"
    INCLUDE "../cli/move.inc"
    INCLUDE "../cli/tree.inc"
    INCLUDE "../dos/ui/toolbar.inc"
    INCLUDE "../cli/data.inc"
    INCLUDE "../cli/moveDirectories.inc"
    INCLUDE "../cli/copyConfirm.inc"
    INCLUDE "../cli/changes.inc"
    ORG_PAD DOS_MODULE_ADDR
moduleHeader:
    jp moduleReturn
    DB "DM",DOS_MODULE_VERSION
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
    jp directoryGetCwd
    jp directoryCommand
    jp cliService
    jp directoryCreate
    jp directoryRemove
    jp cliResolveExternal
    jp directoryExtend
moduleReturn:
    ret
    INCLUDE "../dos/ui/draw.inc"
    INCLUDE "../dos/ui/inverse.inc"
    INCLUDE "../dos/ui/copyRect.inc"
    INCLUDE "../dos/fileAssociation.inc"
    INCLUDE "../dos/fileCopy.inc"
    INCLUDE "../dos/fileHandle.inc"
    INCLUDE "../dos/directoryCwd.inc"
    INCLUDE "../cli/directoryExtend.inc"
moduleEnd:
    ASSERT $ <= DOS_MODULE_END
    END
