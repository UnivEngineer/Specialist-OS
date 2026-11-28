biosRamfosHigh:
    ; F800h–F823h получают прямые назначения из уже установленного C800h.
    DW ramfosCancel,ramfosCancel,calcCS,ramfosFileType
    DW ramfosGetTop,ramfosSetTop,ramfosReadAny,ramfosWriteAny,ramfosSetCursor,ramfosNoFile,ramfosNoFile,ramfosNoFile
    DW ramfosNoFile,ramfosNoFile,ramfosNoFile,ramfosNoFile,biosCompatExit,ramfosNoFile,ramfosStart, biosCompatExit
biosRamfosHighEnd:
    ASSERT biosRamfosHighEnd-biosRamfosHigh == 40
