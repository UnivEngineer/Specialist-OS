; Создано из source/bios/syscalls.json; исторические файловые входы BIOS.
; Регистры передаются через общий диспетчер, выбор main/банка един.
sysLegacyStart:
sysLegacy_fileList:
    MXOS_CALL MXOS_DOS_FILE_LIST
    ret
sysLegacy_fileGetSetDrive:
    MXOS_CALL MXOS_DOS_FILE_GET_SET_DRIVE
    ret
sysLegacy_fileCreate:
    MXOS_CALL MXOS_DOS_FILE_CREATE
    ret
sysLegacy_fileLoad:
    MXOS_CALL MXOS_DOS_FILE_LOAD
    ret
sysLegacy_fileDelete:
    MXOS_CALL MXOS_DOS_FILE_DELETE
    ret
sysLegacy_fileRename:
    MXOS_CALL MXOS_DOS_FILE_RENAME
    ret
sysLegacy_fileLoadInfo:
    MXOS_CALL MXOS_DOS_FILE_LOAD_INFO
    ret
sysLegacy_imageOpen:
    MXOS_CALL MXOS_DOS_IMAGE_OPEN
    ret
sysLegacy_fileGetSetAttr:
    MXOS_CALL MXOS_DOS_FILE_GET_SET_ATTR
    ret
sysLegacy_fileNamePrepare:
    MXOS_CALL MXOS_DOS_FILE_NAME_PREPARE
    ret
sysLegacy_fileExec:
    MXOS_CALL MXOS_DOS_FILE_EXEC
    ret
sysLegacy_fileLoad2:
    MXOS_CALL MXOS_DOS_FILE_LOAD2
    ret
sysLegacy_getDriveFreeSpace:
    MXOS_CALL MXOS_DOS_GET_DRIVE_FREE_SPACE
    ret
sysLegacy_fileCreatePrepare:
    MXOS_CALL MXOS_DOS_FILE_CREATE_PREPARE
    ret
sysLegacy_installVolume:
    MXOS_CALL MXOS_DOS_INSTALL_VOLUME
    ret
sysLegacy_getVolume:
    MXOS_CALL MXOS_DOS_GET_VOLUME
    ret
sysLegacy_mountVolume:
    MXOS_CALL MXOS_DOS_MOUNT_VOLUME
    ret
sysLegacy_flushVolume:
    MXOS_CALL MXOS_DOS_FLUSH_VOLUME
    ret
sysLegacy_fileAssociation:
    MXOS_CALL MXOS_DOS_FILE_ASSOCIATION
    ret
sysLegacy_fileChangeDrive:
    MXOS_CALL MXOS_DOS_FILE_SELECT_DRIVE
    ret
sysLegacy_fileCopy:
    MXOS_CALL MXOS_DOS_FILE_COPY
    ret
sysLegacy_fileGetDrives:
    MXOS_CALL MXOS_DOS_FILE_GET_DRIVES
    ret
sysLegacy_dosGetError:
    MXOS_CALL MXOS_DOS_GET_ERROR
    ret
sysLegacy_logicalSectorIO:
    MXOS_CALL MXOS_DOS_SECTOR_IO
    ret
sysLegacy_fileStream:
    MXOS_CALL MXOS_DOS_FILESTREAM
    ret
sysLegacy_fileHandle:
    MXOS_CALL MXOS_DOS_FILEHANDLE
    ret
sysLegacy_directoryService:
    MXOS_CALL MXOS_DOS_DIRECTORY
    ret
sysLegacy_cliService:
    MXOS_CALL MXOS_DOS_CLI
    ret
sysLegacy_setShell:
    MXOS_CALL MXOS_DOS_SET_SHELL
    ret
sysLegacyEnd:
