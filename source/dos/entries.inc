; Плотная таблица DOS. Адреса объявлены в mxos.inc, реализации перемещаемы.
    ASSERT $ == dos_dosBoot
    jp dosBoot
    ASSERT $ == dos_fileList
    jp fileList
    ASSERT $ == dos_fileGetSetDrive
    jp fileGetSetDrive
    ASSERT $ == dos_fileCreate
    jp fileCreate
    ASSERT $ == dos_fileLoad
    jp fileLoad
    ASSERT $ == dos_fileDelete
    jp fileDelete
    ASSERT $ == dos_fileRename
    jp fileRename
    ASSERT $ == dos_fileLoadInfo
    jp fileLoadInfo
    ASSERT $ == dos_imageOpen
    jp imageOpen
    ASSERT $ == dos_fileGetSetAttr
    jp fileGetSetAttr
    ASSERT $ == dos_fileNamePrepare
    jp fileNamePrepare
    ASSERT $ == dos_fileExec
    jp fileExec
    ASSERT $ == dos_fileLoad2
    jp fileLoad2
    ASSERT $ == dos_getDriveFreeSpace
    jp getDriveFreeSpace
    ASSERT $ == dos_fileCreatePrepare
    jp fileCreatePrepare
    ASSERT $ == dos_dosUnsupported
    jp dosUnsupported
    ASSERT $ == dos_installVolume
    jp installVolume
    ASSERT $ == dos_getVolume
    jp getVolume
    ASSERT $ == dos_mountVolume
    jp mountVolume
    ASSERT $ == dos_flushVolume
    jp flushVolume
    ASSERT $ == dos_fileAssociation
    jp fileAssociation
    ASSERT $ == dos_fileChangeDrive
    jp fileChangeDrive
    ASSERT $ == dos_fileCopy
    jp fileCopy
    ASSERT $ == dos_fileGetDrives
    jp fileGetDrives
    ASSERT $ == dos_dosGetError
    jp dosGetError
    ASSERT $ == dos_logicalSectorIO
    jp logicalSectorIO
    ASSERT $ == dos_fileStream
    jp fileStream
    ASSERT $ == dos_fileHandle
    jp fileHandle
    ASSERT $ == dos_directoryService
    jp directoryService
    ASSERT $ == dos_cliService
    jp cliService
    ASSERT $ == dos_setShell
    jp setShell
