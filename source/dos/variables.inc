; Runtime storage in the DOS bank. Only the cold-start template is
; emitted. The reference linker keeps its own initialized fixture data.
    IFDEF BANKED_DOS_BUILD
dosColdDefaults:
    DISP DOS_VARIABLES_ADDR
    ENDIF
dosWorkStart:

; arena
    IFDEF BANKED_DOS_BUILD
arenaPolicy:    DB IMAGE_POLICY_MANAGED
    ENDIF

; batStream
v_batFrame:     DW BAT_BUFFER-BAT_FRAME_SIZE

; fatCache
v_cacheTable:           DW     cacheData
v_cachePartial:         DW     partialCacheData
v_cacheBuffer:          DW     FAT_CACHE_ADDR
v_cachePartialBuffer:   DW     DOS_BANK_PARTIAL_ADDR
v_cacheLastSlot:        DW     cacheData
v_cacheLastTag:         DB     0FFh
; Четыре последние головы перепроверяются по LBA и тегу.
v_cacheHotSlots:        DW     cacheData,cacheData,cacheData,cacheData
v_cacheStride:          DW     128
v_cacheCount:           DB     CACHE_GRANULES
v_cacheDrive:           DB     0FFh
v_cacheTag:             DB     0FFh
v_cacheSpan:            DB     1
v_cacheMask:            DB     0FFh

; fileFind
v_findMask: DB 018h

; fileList
v_listMask: DB 018h

; kernel-enumerate
    IFDEF BANKED_DOS_BUILD
eqNextHandle:   DW 1

; kernel-fileops
fileopsNext:    DW 1
fileopsPathStamp: DB 0FFh

; kernel-localio
klRamCopy: DB 0
    DW 0
    DB 0
    DW 0,256
klFastOwner: DB DOS_IO_PAGE
klFlushRegs: DW 0,0,(4 << 8)+EXTERNAL_DRIVE,0

; kernel-stream
ksRouteDrive:       DB 0FFh
ksRouteBuffered:    DB 1

; lifecycle
    IFDEF MX_FLOPPY_BOOT
residentShellKey:       DW 0                  ; MAIN до явного запуска банкового NC
    ELSE
residentShellKey:       DW NC_BANK_KEY
    ENDIF

; memory-resident
memoryPublishedEnd: DW APP_MEMORY_END
memorySystemEnd:    DW APP_MEMORY_END

; module-main-claim
mmcPolicy:      DB IMAGE_POLICY_MANAGED

; module-reuse
mdReuseRegs:    DB 0,0,0,0,0,0,0,MODULE_ACTIVATE

; modules
mdNext:         DW 1
mdCopy:         DB 1
    DW 0
    DB 0
    DW 0,0
mdContext: DB 0,16,0,0,0
    DW DOS_BANK_STACK_TOP,DOS_BANK_STACK_BOTTOM+64
mdLocalContext: DW 0
    DB MEM_BANK_EXEC
    DW 0
mdNearCopy: DW bankMemExecCopy
mdExecReady: DB MEM_EXEC_READY_VALUE
mdDataKind: DB MEM_BANK_DATA

; overlays
;----------------------------------------------------------------------------
; MXOS
; Передать регистры в опубликованный оверлей FILEOPS или ENUM.
; вход: ready=1, фасад удерживает оверлей, AF/BC/DE/HL — аргументы операции.
; выход: регистры и флаги возвращает движок, стек вызывающего восстановлен.
;----------------------------------------------------------------------------
fileopsOverlay:
    DW 0
fileopsInvoke:
    jp 0                     ; Операнд задаёт overlayEnsure.
    DW fileopsName,fileopsHost
    DB fileopsHostEnd-fileopsHost,0,0
    DW 0
enumOverlay:
    DW 0
enumInvoke:
    jp 0                     ; Операнд задаёт overlayEnsure.
    DW enumName,enumHost
    DB enumHostEnd-enumHost,0,0
    DW 0

fileopsReady     = fileopsOverlay+OV_READY
fileopsActive    = fileopsOverlay+OV_ACTIVE
fileopsLoadedEnd = fileopsOverlay+OV_END
enumReady     = enumOverlay+OV_READY
enumActive    = enumOverlay+OV_ACTIVE
enumLoadedEnd = enumOverlay+OV_END

; ramdisk
dosRamCopy: DB 0
    DW 0
    DB 0
    DW 0,256
    ENDIF

; state
v_batEcho:          DB 1  ; BAT печатает команды, пока не выполнено ECHO OFF.

; biosState
    IFDEF BANKED_DOS_BUILD
v_memTop: DW DOS_CODE_START-1
kernelRamDrive: DB RAM_DRIVE
kernelFloppyDrive: DB 0FFh
kernelExternalDrive: DB 0FFh
v_drive:        DB RAM_DRIVE
v_drives:       DW diskDriverDummy, diskDriver      ; A:, B:
    IFDEF MX_FLOPPY_BOOT
                DW diskDriverDummy, diskDriverDummy ; C: подключается командой SYSROM
    ELSE
                DW diskDriver, diskDriverDummy      ; C:, D:
    ENDIF
                    DW diskDriverDummy, diskDriverDummy  ; E:, F:
                    DW diskDriverDummy, diskDriverDummy  ; G:, H:

; installDriver
v_blockShifts: DB 8,8,8,8,8,8,8,8
    ENDIF

; PATH занимает MAX_PATH байт и переживает тёплый перезапуск.
v_cliPath: DB BOOT_VOLUME_LETTER,':',05Ch,"DOS",0
dosDefaultsEnd:
    IFDEF BANKED_DOS_BUILD
    ENT
    ENDIF

    MACRO DOS_FIELD field, bytes
field = dosWorkNext
dosWorkNext = dosWorkNext + bytes
    ENDM
dosWorkNext = dosDefaultsEnd + MAX_PATH-7

; arena
    IFDEF BANKED_DOS_BUILD
    DOS_FIELD relocHoleSpan, 2
    DOS_FIELD relocHoleMask, 2
    DOS_FIELD relocHoleMain, 1
    DOS_FIELD relocHolePass, 1
    DOS_FIELD appHeapBounds, APP_MAX_DEPTH*4
    DOS_FIELD arenaCacheRecord, 2
    DOS_FIELD arenaStart, 2
    DOS_FIELD arenaEnd, 2
    DOS_FIELD arenaPhase, 1

    DOS_FIELD v_fat12, 1
    DOS_FIELD f12Mode, 1
    DOS_FIELD f12Odd, 1
    DOS_FIELD f12First, 2
    DOS_FIELD f12Second, 2
    DOS_FIELD f12Slot, 2
    DOS_FIELD f12New, 2
    ENDIF

; batStream
    DOS_FIELD batLeft, 2        ; Непрочитанный хвост общего окна.
    DOS_FIELD batPtr, 2
    DOS_FIELD batWindowEnd, 2   ; Ноль при потоковом чтении вне арены.
    IFDEF BANKED_DOS_BUILD
    DOS_FIELD arenaObstacle, 2  ; Начало препятствия при поиске места сверху.
    ENDIF
    DOS_FIELD v_batDepth, 1
    DOS_FIELD v_batReported, 1

; cli-data
    DOS_FIELD cliSavedDirectories, 16
    DOS_FIELD cliOldDrive, 1
    DOS_FIELD cliOldValid, 1
    DOS_FIELD cliOldDirectory, 2
    DOS_FIELD cliMoveFlag, 1
    DOS_FIELD cliParent, 2
    DOS_FIELD cliNewCluster, 2
    DOS_FIELD cliRemaining, 2
    DOS_FIELD cliCursor, FILE_STREAM_CURSOR
cliRequest FILE_COPY_REQUEST = dosWorkNext
dosWorkNext = dosWorkNext + FILE_COPY_REQUEST

; command-data
    DOS_FIELD cliOutput, 1
    DOS_FIELD cliQuery, 1
    DOS_FIELD cliAttribSet, 1
    DOS_FIELD cliAttribClear, 1
    DOS_FIELD cliArgs, 2
    DOS_FIELD cliTail, 2
    DOS_FIELD cliExtPtr, 2
    DOS_FIELD cliSearchDrive, 1
    DOS_FIELD cliNeedExtension, 1
    DOS_FIELD cliPathCursor, 2
    DOS_FIELD cliLine, MAX_PATH
    DOS_FIELD cliCopyHook, 2
    DOS_FIELD cliDirRecursive, 1
    DOS_FIELD cliTransferMatched, 1

; directory
    DOS_FIELD v_dirRemaining, 1
    DOS_FIELD v_dirRootRemaining, 2
    DOS_FIELD v_cachedDescrOffset, 2
    DOS_FIELD v_dirCluster, 2
    DOS_FIELD v_dirSectorsLeft, 2
    DOS_FIELD v_dirBudget, 2

; directoryCwd
    DOS_FIELD cwdBuffer, 2
    DOS_FIELD cwdWrite, 2
    DOS_FIELD cwdChild, 2
    DOS_FIELD cwdLength, 1

; directoryService
    DOS_FIELD v_pathEnd, 2
    DOS_FIELD v_directory, 2
    DOS_FIELD v_directories, 16
    DOS_FIELD v_pathOld, 2
    DOS_FIELD v_pathName, 11

; enumerate
    IFDEF BANKED_DOS_BUILD
    DOS_FIELD enumOperation, 1
    DOS_FIELD enumDrive, 1
    DOS_FIELD enumDirectory, 2
    DOS_FIELD enumDirectories, 16
    ENDIF

; errors
    DOS_FIELD v_dosError, 1
    DOS_FIELD fatWriteError, 1

; fatCache
    DOS_FIELD v_cacheProbeCount, 1
    DOS_FIELD v_cacheMaxSector, 2
    DOS_FIELD v_cacheHotNext, 1
    DOS_FIELD v_cacheUsed, 1
    DOS_FIELD v_cacheDirty, 1
    DOS_FIELD v_cacheDirtySlot, 2
    DOS_FIELD v_cacheShift, 1
    DOS_FIELD v_cacheGeometries, 8
    DOS_FIELD v_cacheMaxByDrive, 16
    DOS_FIELD v_cacheClock, 1
    DOS_FIELD v_allocIndex, 1
    DOS_FIELD v_allocCursor, 1
    DOS_FIELD v_allocEnd, 1
    DOS_FIELD v_invalidateDrive, 1

; fatGetFreeSpace
    DOS_FIELD v_freeSector, 2
    DOS_FIELD v_freeEnd, 2

; fatReadBootSector
    IFDEF BANKED_DOS_BUILD
    ; Геометрия заполняется при первом монтировании; в образе DOS ей места не нужно.
v_diskInfo DISK_INFO = dosWorkNext
dosWorkNext = dosWorkNext + DISK_INFO
    ENDIF
    DOS_FIELD v_logicalCapacity, 2
    DOS_FIELD v_logicalShift, 1

; fileCopy
; Результаты FILEOPS для межтомного MOVE.
    DOS_FIELD treeMoveMode, 1
    DOS_FIELD treeHandled, 1
    DOS_FIELD copySkipped, 1

; fileHandle
    DOS_FIELD v_handle, 2
    DOS_FIELD v_handleOp, 1
    DOS_FIELD v_handleLeft, 2
    DOS_FIELD v_handleDone, 2
    DOS_FIELD v_handleChunk, 2
    DOS_FIELD v_handleBuffer, 2
    DOS_FIELD v_handleDelta, 4
    DOS_FIELD v_handleCluster, 2
    DOS_FIELD v_handlePrevious, 2
    DOS_FIELD v_handleSign, 1
    DOS_FIELD v_handleEOF, 1
    DOS_FIELD v_handleNew, 2
    DOS_FIELD v_handleOldTail, 2
    DOS_FIELD v_handleWalk, 2

; fileStream
    DOS_FIELD v_streamOffset, 2
    DOS_FIELD v_streamRemaining, 2
    DOS_FIELD v_streamPtr, 2
    DOS_FIELD v_streamCluster, 2
    DOS_FIELD v_streamMode, 1
    DOS_FIELD v_streamChunk, 2
    DOS_FIELD v_streamSector, 2
    DOS_FIELD v_streamWithin, 2
    DOS_FIELD v_streamPrevious, 2
    DOS_FIELD v_streamChecked, 1
    DOS_FIELD v_streamFileWrite, 1

; handle-options
    DOS_FIELD v_fileOptions, 1
    DOS_FIELD v_handleChecked, 1

; kernel-copy
    IFDEF BANKED_DOS_BUILD
    DOS_FIELD kcHook, 2

; kernel-dispatch
    DOS_FIELD kdBusy, 1
    DOS_FIELD kdPage, 1
    DOS_FIELD kdFunction, 1
    DOS_FIELD kdOperation, 1
    DOS_FIELD kdIOStarted, 1

; kernel-enumerate
    DOS_FIELD eqOwner, 1
    DOS_FIELD eqHandle, 2
    DOS_FIELD eqOperation, 1
    DOS_FIELD eqLeft, 1
    DOS_FIELD eqBuffer, 2

; kernel-exec
    DOS_FIELD keRegs, MX_REGS
    DOS_FIELD keCacheToken, 2
    DOS_FIELD keImageToken, 2

; kernel-fileops
    DOS_FIELD fileopsOp, 1
    DOS_FIELD fileopsPublic, 1
    DOS_FIELD fileopsOwner, 1
    DOS_FIELD fileopsHandle, 2

; kernel-image
    DOS_FIELD kiSumActive, 1
    DOS_FIELD kiSum, 2

; kernel-list
    DOS_FIELD klActive, 1
    DOS_FIELD klFailed, 1
    DOS_FIELD klCursor, 2

; kernel-localio
    DOS_FIELD klRequestReady, 1
    DOS_FIELD klInstalled, 17
    DOS_FIELD klMarker, 2
    DOS_FIELD klProbe, 1
    DOS_FIELD klRegs, MX_REGS
    DOS_FIELD klLease, 3

; kernel-meta
    DOS_FIELD kmWrite, 1
    DOS_FIELD kmSize, 1
    DOS_FIELD kmSecond, 2

; kernel-observer
    DOS_FIELD koAddress, 2
    DOS_FIELD koPage, 1

; kernel-registry
    DOS_FIELD registrySystemText, 2
    DOS_FIELD diStart, 2
    DOS_FIELD diActive, 1
    DOS_FIELD diEnd, 2
    DOS_FIELD diBodyStart, 2
    DOS_FIELD diBodyEnd, 2
    DOS_FIELD diNewBase, 2
    DOS_FIELD diNewEnd, 2
    DOS_FIELD diNewEntry, 2
    DOS_FIELD diFixCount, 2
    DOS_FIELD diDescriptor, 2
    DOS_FIELD diImage, 2
    DOS_FIELD diSize, 2
    DOS_FIELD diText, 2
    DOS_FIELD diTextSize, 1
    DOS_FIELD registryImageSize, 2

; kernel-start
    DOS_FIELD v_dosModuleReady, 1

; kernel-stream
    DOS_FIELD kdFarActive, 1
    DOS_FIELD kdFarCurrent, 2
    DOS_FIELD ksMode, 1
    DOS_FIELD ksLeft, 2
    DOS_FIELD ksChunk, 2
    DOS_FIELD ksActual, 2
    DOS_FIELD ksResult, 2
    DOS_FIELD ksDirect, 1
    DOS_FIELD ksDirectStart, 2

; kernel-system
    DOS_FIELD krOperation, 1
    DOS_FIELD ktHook, 2
    DOS_FIELD ktRegs, MX_REGS
    DOS_FIELD krSectorSize, 2
    DOS_FIELD kernelHDriverKind, 1
    DOS_FIELD diFloppyStart, 2
    DOS_FIELD diFloppyEnd, 2
    DOS_FIELD ramRequestedDrive, 1

; lifecycle
dosWorkBeforeResident = dosWorkNext
dosWorkNext = DOS_RESIDENT_WORK
    DOS_FIELD residentShellHandle, 2
    DOS_FIELD appOwner, 1
    DOS_FIELD appDepth, 1
    DOS_FIELD appImage, 2
    DOS_FIELD residentKeys, MODULE_MAX*2
    DOS_FIELD residentIndex, 1
    DOS_FIELD residentPriorities, MODULE_MAX
    DOS_FIELD residentVetoCodes, MODULE_MAX
    DOS_FIELD residentVetoTexts, MODULE_MAX*2
    DOS_FIELD residentVetoFlags, MODULE_MAX*2
    DOS_FIELD residentManaged, MODULE_MAX
    DOS_FIELD residentReloadSlots, MODULE_MAX
    DOS_FIELD residentReloadStatus, pressureSlotCount
    DOS_FIELD pressureTried, MODULE_MAX
    DOS_FIELD pressureBest, 1
    DOS_FIELD pressureIndex, 1
    DOS_FIELD pressureRefusal, 1
    DOS_FIELD pressureBusy, 1
    DOS_FIELD pressureRecord, 2
    DOS_FIELD pressurePages, 1
    DOS_FIELD pressureCommandReady, 1
    DOS_FIELD pressureStateSize, 1
    DOS_FIELD pressureState, RESIDENT_STATE_BYTES
    DOS_FIELD pressurePaths, pressureSlotCount*PRESSURE_PATH_BYTES
    DOS_FIELD pressureRefusalName, 2
    DOS_FIELD pressureRefusalText, 2
    DOS_FIELD pressureRefusalBank, 1

; memory-extra
    DOS_FIELD memoryFillSize, 2
    DOS_FIELD memoryFillDone, 2
    DOS_FIELD memoryFillCopy, 8
    DOS_FIELD memoryQueryBank, 1
    DOS_FIELD memoryQueryCursor, 2
    DOS_FIELD memoryQueryEnd, 2
    DOS_FIELD memoryQueryNext, 2
    DOS_FIELD memoryQueryBlockEnd, 2
    DOS_FIELD memoryQueryTotal, 2
    DOS_FIELD memoryQueryMax, 2

; memory-resident
    DOS_FIELD mrSlots, MEM_RESIDENT_SLOTS*4
    DOS_FIELD mrSlot, 2
    DOS_FIELD mrSize, 2
    DOS_FIELD mrFloor, 2
    DOS_FIELD mrBase, 2
    DOS_FIELD mrCeiling, 2
    DOS_FIELD mrOperation, 1

; memory
    DOS_FIELD timerOwner, 1
    DOS_FIELD memoryRamOwner, 1
    DOS_FIELD memoryRamPages, 1
    DOS_FIELD memoryRequestedPages, 2
    DOS_FIELD memoryMainOwner, 1
    DOS_FIELD memoryMainStart, 2
    DOS_FIELD memoryMainEnd, 2
    DOS_FIELD memoryPlacement, 1
    DOS_FIELD memoryBank, 1
    DOS_FIELD memorySize, 2
    DOS_FIELD memoryCandidate, 2
    DOS_FIELD memoryLimit, 2
    DOS_FIELD memoryNewEnd, 2
    DOS_FIELD memoryBlockEnd, 2
    DOS_FIELD memorySlotPtr, 2
    DOS_FIELD memoryOldSlot, 2
    DOS_FIELD memoryOldBank, 1
    DOS_FIELD memoryOldAddress, 2
    DOS_FIELD memoryOldSize, 2
    DOS_FIELD memoryResize, 2
    DOS_FIELD memoryCopy, 8

; module-main-claim
    DOS_FIELD mmcStart, 2
    DOS_FIELD mmcEnd, 2
    DOS_FIELD mmcOwner, 1
    DOS_FIELD mmcPhase, 1
    DOS_FIELD mmcAllowOwner, 1
    DOS_FIELD mfCaller, 1

; module-reuse
    DOS_FIELD mdReuseHit, 1
    DOS_FIELD mdReuseSpare, 2
    DOS_FIELD mdReuseBegin, 2
    DOS_FIELD mdReuseEnd, 2
    DOS_FIELD mdReuseKey, 7
    DOS_FIELD mdReuseKeys, (MODULE_MAX-1)*8+7

; modules
    DOS_FIELD moduleActor, 1
    DOS_FIELD mdRecord, 2
    DOS_FIELD mdClientRegs, 2
    DOS_FIELD mdCaller, 1
    DOS_FIELD mdOwner, 1
    DOS_FIELD mdPlacement, 1
    DOS_FIELD mdBankId, 1
    DOS_FIELD mdAddress, 2
    DOS_FIELD mdEntry, 2
    DOS_FIELD mdMemoryStart, 2
    DOS_FIELD mdMemoryEnd, 2
    DOS_FIELD mdCallEntry, 2

    INCLUDE "module-reloc-vars.inc"

; overlays
    DOS_FIELD ovRecord, 2
    DOS_FIELD ovImageEnd, 2
    DOS_FIELD ovDirectories, 16

; romproxy
    DOS_FIELD dosSystemRequest, 5
dosResidentWorkEnd = dosWorkNext
    ASSERT dosResidentWorkEnd <= DOS_ARENA_START
dosWorkNext = dosWorkBeforeResident
    ENDIF

; services
    DOS_FIELD v_shell, 1
    DOS_FIELD v_shellDrive, 1

; state
    DOS_FIELD v_findCluster, 2
    DOS_FIELD v_fileFirstCluster, 2
    DOS_FIELD v_cachedDescrPtr, 2
    DOS_FIELD v_cachedSector, 2
    DOS_FIELD v_newDescrPtr, 2
    DOS_FIELD v_foundDescrPtr, 2
    DOS_FIELD v_execDrive, 1
    DOS_FIELD v_bootAutoexec, 1
    DOS_FIELD v_dirFirstFile, 2
    DOS_FIELD v_dirMaxFiles, 2
    DOS_FIELD v_dirTotalFiles, 2
    DOS_FIELD v_fakeSystemTime, 2    ; Счётчик обращений вместо RTC.
    DOS_FIELD v_curFileDescr, FILE_NAME_LENGTH+3
v_tmpFileDescr FILE_DESCRIPTOR = dosWorkNext
dosWorkNext = dosWorkNext + FILE_DESCRIPTOR
; fileLoadExit копирует перекрывающиеся дескрипторы вперёд.
    ASSERT v_curFileDescr+FILE_DESCR_SIZE <= v_tmpFileDescr+FILE_DESCR_SIZE

; changes
    DOS_FIELD cliChangeHook, 2
    DOS_FIELD cliChangeEpoch, 4
    DOS_FIELD cliDriveEpochs, 8*4

; directoryExtend
    DOS_FIELD directoryNewCluster, 2
    DOS_FIELD directoryNewSector, 2
    DOS_FIELD directorySectorsLeft, 2

; moveDirectories
    DOS_FIELD cliMoveBudget, 2

; biosState
    IFDEF BANKED_DOS_BUILD
    DOS_FIELD v_input_start, 2
    DOS_FIELD v_input_end, 2

; BOOTBANK writes this byte before kernelCold. It survives BSS initialization.
dosZeroEnd = dosWorkNext
    DOS_FIELD memoryRamFresh, 1
    IFDEF MX_FLOPPY_BOOT
    DOS_FIELD mxBootCookie, 6   ; Носитель, источник комплекта и идентификатор сборки
    ENDIF
    ENDIF
dosWorkEnd = dosWorkNext
    IFDEF BANKED_DOS_BUILD
    ASSERT dosWorkEnd <= DOS_BANK_DATA_ADDR
    ELSE
    DS dosWorkEnd-$,0
    ENDIF
