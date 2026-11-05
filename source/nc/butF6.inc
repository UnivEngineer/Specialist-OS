; F6 — перемещение: на одном томе меняются только записи каталога.
but_F6:
            ld a,(ncShiftState)
            or a
            jp z,ncRename
            ld a,CLI_OP_MOVE
            ld (copyFileOperation+1),a
            call drawWindowIfSel
            CENTER_LINE 7, 5Eh
            ld hl,ncMoveText
            call bios_printString
            call inputForCopyMove
            cp 01Bh
            jp z,ncRefreshDirty
            call copyFileInt
            jp ncRefreshDirty
ncMoveText: DB 'Move to',0

; Shift+F6 меняет только имя в текущем каталоге, без выбора второй панели.
ncRename:
            call drawWindowIfSel
            CENTER_LINE 9, 5Eh
            ld hl,ncRenameText
            call bios_printString
            call inputForRename
            cp 01Bh
            jp z,ncRefreshDirty
            ld hl,state.panelA_drive
            call readBytePanel
            call bios_fileSelectDrive
            jp c,copyFileError
            call restorePanelDirectory
            ld hl,vars.input
ncRenameLeaf:
            ld a,(hl)
            or a
            jp z,ncRenameLeafReady
            cp ':'
            jp z,ncRenameBadPath
            cp '/'
            jp z,ncRenameBadPath
            cp 05Ch
            jp z,ncRenameBadPath
            inc hl
            jp ncRenameLeaf
ncRenameLeafReady:
            ld hl,vars.input
            ld de,vars.copyRequest.destination
            call bios_fileNamePrepare
            jp c,copyFileError
            call getSelectedFile
            ld de,vars.copyRequest.destination
            ld a,CLI_OP_RENAME
            call bios_cli
            jp c,copyFileError
            jp ncRefreshDirty
ncRenameText: DB 'Rename to',0

ncRenameBadPath:
            ld a,DOS_ERR_ARGUMENT
            jp copyFileErrorCode
