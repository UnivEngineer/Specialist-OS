; Плотная таблица DOS. Адреса объявлены в mxos.inc, реализации перемещаемы.
    ASSERT $ == dos_dosBoot
    jp dosBoot
    ASSERT $ == dos_fileList
    jp fileList
    ASSERT $ == dos_fileGetSetDrive
    jp fileGetSetDrive
    DS 3,0 ; Reserved; file I/O uses FILEHANDLE
    DS 3,0 ; Reserved; file I/O uses FILEHANDLE
    ASSERT $ == dos_fileDelete
    jp fileDelete
    ASSERT $ == dos_fileRename
    jp fileRename
    ASSERT $ == dos_fileLoadInfo
    jp fileLoadInfo
    DS 3,0 ; Reserved; file I/O uses FILEHANDLE
    ASSERT $ == dos_fileGetSetAttr
    jp fileGetSetAttr
    ASSERT $ == dos_fileNamePrepare
    jp fileNamePrepare
    ASSERT $ == dos_fileExec
    jp fileExec
    DS 3,0 ; Reserved; file I/O uses FILEHANDLE
    ASSERT $ == dos_getDriveFreeSpace
    jp getDriveFreeSpace
    DS 3,0 ; No public preallocation facade; creation uses FILEHANDLE
    ASSERT $ == dos_dosUnsupported
    jp dosUnsupported
    DS 6,0 ; No alternative filesystem registration facade
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
    DS 3,0 ; Reserved; file I/O uses FILEHANDLE
    ASSERT $ == dos_fileHandle
    jp fileHandle
    ASSERT $ == dos_directoryService
    jp directoryService
    ASSERT $ == dos_cliService
    jp cliService
    ASSERT $ == dos_setShell
    jp setShell
