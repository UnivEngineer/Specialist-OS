; Создано из source/bios/syscalls.json; исторические файловые входы BIOS.
; Регистры передаются через общий диспетчер, выбор main/банка един.
sysLegacyStart:
sysLegacy_fileList:
    MXCALL MX_DOS_FILELIST
    ret
sysLegacy_fileGetSetDrive:
    MXCALL MX_DOS_FILEGETSETDRIVE
    ret
sysLegacy_fileCreate:
    MXCALL MX_DOS_FILECREATE
    ret
sysLegacy_fileLoad:
    MXCALL MX_DOS_FILELOAD
    ret
sysLegacy_fileDelete:
    MXCALL MX_DOS_FILEDELETE
    ret
sysLegacy_fileRename:
    MXCALL MX_DOS_FILERENAME
    ret
sysLegacy_fileLoadInfo:
    MXCALL MX_DOS_FILELOADINFO
    ret
sysLegacy_imageOpen:
    MXCALL MX_DOS_IMAGEOPEN
    ret
sysLegacy_fileGetSetAttr:
    MXCALL MX_DOS_FILEGETSETATTR
    ret
sysLegacy_fileNamePrepare:
    MXCALL MX_DOS_FILENAMEPREPARE
    ret
sysLegacy_fileExec:
    MXCALL MX_DOS_FILEEXEC
    ret
sysLegacy_fileLoad2:
    MXCALL MX_DOS_FILELOAD2
    ret
sysLegacy_getDriveFreeSpace:
    MXCALL MX_DOS_GETDRIVEFREESPACE
    ret
sysLegacy_fileCreatePrepare:
    MXCALL MX_DOS_FILECREATEPREPARE
    ret
sysLegacy_dosUnsupported:
    MXCALL MX_DOS_RESERVEDC88D
    ret
sysLegacy_installVolume:
    MXCALL MX_DOS_INSTALLVOLUME
    ret
sysLegacy_getVolume:
    MXCALL MX_DOS_GETVOLUME
    ret
sysLegacy_mountVolume:
    MXCALL MX_DOS_MOUNTVOLUME
    ret
sysLegacy_flushVolume:
    MXCALL MX_DOS_FLUSHVOLUME
    ret
sysLegacy_fileAssociation:
    MXCALL MX_DOS_FILEASSOCIATION
    ret
sysLegacy_fileChangeDrive:
    MXCALL MX_DOS_FILESELECTDRIVE
    ret
sysLegacy_fileCopy:
    MXCALL MX_DOS_FILECOPY
    ret
sysLegacy_fileGetDrives:
    MXCALL MX_DOS_FILEGETDRIVES
    ret
sysLegacy_dosGetError:
    MXCALL MX_DOS_GETERROR
    ret
sysLegacy_logicalSectorIO:
    MXCALL MX_DOS_SECTORIO
    ret
sysLegacy_fileStream:
    MXCALL MX_DOS_FILESTREAM
    ret
sysLegacy_fileHandle:
    MXCALL MX_DOS_FILEHANDLE
    ret
sysLegacy_directoryService:
    MXCALL MX_DOS_DIRECTORY
    ret
sysLegacy_cliService:
    MXCALL MX_DOS_CLI
    ret
sysLegacy_setShell:
    MXCALL MX_DOS_SETSHELL
    ret
sysLegacyEnd:
