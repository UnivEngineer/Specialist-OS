; Внешний DOS-запрос: A=страница клиента, BC=группа/функция,
; DE=MX_REGS в общем окне. Результат AF/BC/DE/HL, SP сохраняет транспорт.
; NEAR-объекты принадлежат клиенту, локальные копии — только банку DOS.
; Повторный вход не разрешает вторую файловую транзакцию.
KD_OBJECT = DOS_BANK_OBJECT_ADDR
KD_PATH   = DOS_BANK_PATH_ADDR
KD_DATA   = DOS_BANK_DATA_ADDR
KD_CHUNK  = DOS_BANK_DATA_SIZE
;----------------------------------------------------------------------------
; MXOS
; Прочитать ошибку DOS по прямому регистровому маршруту из EXEC.
; вход: нет аргументов, B содержит селектор возврата. Обработчик не вызывает MEM.
; выход: AF как у dosGetError; при занятом диспетчере CF=1, A=DOS_ERR_ACCESS.
; BC, DE, HL, контекст исполнителя и кадр приостановленной службы сохранены.
;----------------------------------------------------------------------------
kdLeafGetError:
    ld a,(kdBusy)
    or a
    jp z,dosGetError
    ld a,DOS_ERR_ACCESS
    scf
    ret

kernelDispatch:
    ; Межстраничный пакет находится в общем окне, даже для инициатора DOS.
    ld de,BANK_FRAME_REGS
    jp kdEntryCommon
kernelLocalDispatch:
    ; DE уже указывает на локальный MX_REGS: транспорт/копировщик не нужны.
    ld a,DOS_IO_PAGE
    ld (BANK_FRAME_STATUS),a
kdEntryCommon:
    ; Nested calls from a directly entered DLL use its allocation owner.
    push af
    ld a,(bankHome)
    cp MODULE_OWNER_BASE
    jp c,kdDefaultOwner
    ld l,a
    ld a,(moduleActor)
    ld h,a
    push hl
    ld a,l
    ld (moduleActor),a
    xor a
    ld (bankHome),a
    ld hl,3
    add hl,sp
    ld a,(hl)
    call kdOwnedEntry
    ex (sp),hl
    push af
    ld a,l
    ld (bankHome),a
    ld a,h
    ld (moduleActor),a
    pop af
    ex (sp),hl
    inc sp
    inc sp
    inc sp
    inc sp
    ret
kdDefaultOwner:
    pop af
kdOwnedEntry:
    push af
    ld a,(kdBusy)
    or a
    jp nz,kdReentry
    pop af
    ld (kdPage),a
    xor a
    ld (klRequestReady),a
    ld a,0FFh
    ld (ksRouteDrive),a
    ld a,b
    cp MXOS_GROUP_DOS
    jp nz,dosUnsupported
    ld a,c
    ld (kdFunction),a
    ex de,hl
    ld de,kdRegs
    ld b,MX_REGS
kdPacketLoop:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,kdPacketLoop
    ld a,1
    ld (kdBusy),a
    ld a,(kdFunction)
    cp MXOS_DOS_MODULE_SCOPE & 255
    jp z,moduleScope
    cp MXOS_DOS_COMMAND_BATCH & 255
    jp z,dosCommandBatch
    cp MXOS_DOS_ENUMERATE & 0FFh
    jp z,enumService
    cp MXOS_DOS_FILEOPS & 0FFh
    jp z,fileopsService
    cp MXOS_DOS_RESIDENT & 0FFh
    jp z,residentService
    cp MXOS_DOS_MODULE_LOAD & 0FFh
    jp z,moduleLoad
    cp MXOS_DOS_MODULE_CALL & 0FFh
    jp z,moduleCall
    cp MXOS_DOS_MODULE_UNLOAD & 0FFh
    jp z,moduleUnload
    cp MXOS_DOS_FILE_EXEC_NATIVE & 0FFh
    jp z,keRequest
    cp MXOS_DOS_MEMORY & 0FFh
    jp z,kdLocal
    ld a,(kdPage)
    cp DOS_IO_PAGE
    jp z,kdLocal
    ld a,(kdFunction)
    cp (kdExternalEnd-kdExternalTable)/2
    jp nc,kdUnsupported
    add a,a
    ld l,a
    ld h,0
    ld de,kdExternalTable
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld a,h
    or a
    jp m,kdExternalScalar
    jp (hl)
kdExternalScalar:
    and 07Fh
    ld h,a
    jp kdInvokeScalar
kdLocal:
    ; Локальные драйверы получают те же номера, но исходные NEAR-указатели.
    ld a,(kdFunction)
    cp (kdLocalEnd-kdLocalTable)/2
    jp nc,kdUnsupported
    add a,a
    ld e,a
    ld d,0
    ld hl,kdLocalTable
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
kdInvokeScalar:
    ld (kdEntry),hl
kdDirectoryScalar:
    call kdInvoke
    call kdCapture
    jp kdFinish

kdDirectory:
    ld hl,directoryService
    ld (kdEntry),hl
    ld a,(kdRegs+7)
    cp DIR_OP_GET
    jp z,kdDirectoryScalar
    cp DIR_OP_SET
    jp z,kdDirectoryScalar
    cp DIR_OP_RESOLVED
    jp z,kdDirectoryScalar
    cp DIR_OP_GETCWD
    jp z,kdCwd
    cp DIR_OP_LIST
    jp z,kdList
    cp DIR_OP_CHDIR
    jp z,kdPath
    cp DIR_OP_MKDIR
    jp z,kdPath
    cp DIR_OP_RMDIR
    jp nz,kdUnsupported
kdPath:
    ld hl,(kdRegs)
    ld (kdClientObject),hl
    call kdFetchPath
    jp c,kdBad
    ld hl,KD_PATH
    ld (kdRegs),hl
    call kdInvoke
    call kdCapture
    jp kdRestorePointer
kdCwd:
    ld hl,(kdRegs)
    ld (kdClientObject),hl
    ld bc,MAX_PATH
    call kdRange
    jp c,kdBad
    ld hl,KD_PATH
    ld (kdRegs),hl
    call kdInvoke
    call kdCapture
    ld a,(kdRegs+6)
    and 1
    jp nz,kdRestorePointer
    ld hl,(kdClientObject)
    ex de,hl
    ld hl,KD_PATH
    ld a,(cwdLength)
    inc a
    ld c,a
    ld b,0
    call kdStore
    jp c,kdBad
    jp kdRestorePointer

; Текстовые команды передаются до нуля. Наблюдатель вызывается в
; странице клиента; подтверждения COPY/MOVE требуют отдельного адаптера.
kdCli:
    ld hl,cliService
    ld (kdEntry),hl
    ld a,(kdRegs+7)
    cp CLI_OP_RENAME
    jp z,kmRenameArgs
    cp CLI_OP_COPY
    jp z,kcCopy
    cp CLI_OP_MOVE
    jp z,kcMove
    cp CLI_OP_OBSERVER
    jp z,koRegister
    cp CLI_OP_QUERY
    jp z,kdPath
    cp CLI_OP_ASSOCIATION
    jp z,kdPath
    cp CLI_OP_COMMAND
    jp nz,kdCliPath
    ld hl,(kdRegs+2)
    ld (ktHook),hl
    ld bc,1
    call kdRange
    jp c,kdBad
    ld a,h
    or l
    jp z,kdPath
    ld hl,ktConfirm
    ld (kdRegs+2),hl
    jp kdPath
kdCliPath:
    cp CLI_OP_DELETE
    jp c,kdUnsupported
    cp CLI_OP_RMDIR+1
    jp nc,kdUnsupported
    jp kdPath

; FILE_HANDLE переносится целиком; его FH_BUFFER остаётся NEAR клиента.
; Произвольная длина разбивается на пачки. Ошибка/EOF возвращают общий
; фактически переданный FH_COUNT, включая последнюю неполную пачку.
kdHandle:
    ld hl,(kdRegs)
    ld bc,FILE_HANDLE
    call kdFileRange
    jp c,kdBad
    ld a,(kdRegs+7)
    cp FILE_OP_OPEN_AT+1
    jp nc,kdBad
    ld (kdOperation),a
    ld hl,(kdRegs)
    ld (kdClientObject),hl
    ld de,KD_OBJECT
    ld bc,FILE_HANDLE
    call kdFetch
    jp c,kdBad
    ld a,(kdOperation)
    cp FILE_OP_READ
    jp z,kdHandleIO
    cp FILE_OP_WRITE
    jp z,kdHandleIO
    cp FILE_OP_OPEN_PATH
    jp nz,kdHandleSimple
    ld hl,(KD_OBJECT+FH_BUFFER)
    ld (kdOriginalBuffer),hl
    call kdFetchPath
    jp c,kdBad
    ld hl,KD_PATH
    ld (KD_OBJECT+FH_BUFFER),hl
kdHandleSimple:
    ld a,(kdOperation)
    ld hl,KD_OBJECT
    call fileHandle
    push af
    pop hl
    ld (kdResult),hl
    ld a,(kdOperation)
    cp FILE_OP_OPEN_PATH
    jp nz,kdHandleReturn
    ld hl,(kdOriginalBuffer)
    ld (KD_OBJECT+FH_BUFFER),hl
    jp kdHandleReturn
kdHandleIO:
    ld hl,(KD_OBJECT+FH_BUFFER)
    ld (kdOriginalBuffer),hl
    ld (kdBuffer),hl
    ld hl,(KD_OBJECT+FH_COUNT)
    ld (kdLeft),hl
    ld b,h
    ld c,l
    ld a,b
    or c
    jp z,kdNoOverlap
    ld hl,(kdBuffer)
    call kdFileRange
    jp c,kdBad
    ; Непустой буфер не может перекрывать объект FILE_HANDLE.
    ld a,b
    or c
    jp z,kdNoOverlap
    add hl,bc
    ex de,hl
    ld hl,(kdClientObject)
    call cmp_hl_de
    jp nc,kdNoOverlap
    ld de,FILE_HANDLE
    add hl,de
    ex de,hl
    ld hl,(kdBuffer)
    call cmp_hl_de
    jp c,kdBad
kdNoOverlap:
    ld hl,0
    ld (kdTotal),hl
kdHandleLoop:
    ; Полный диапазон уже проверен. Buffered fallback дробит его в ksBuffered.
    ld hl,(kdLeft)
kdHaveChunk:
    ld (kdChunk),hl
    ld (KD_OBJECT+FH_COUNT),hl
    ld b,h
    ld c,l
    ld hl,(kdBuffer)
    ld (kdFarCurrent),hl
    ld a,1
    ld (kdFarActive),a
kdHandleCall:
    ld hl,KD_DATA
    ld (KD_OBJECT+FH_BUFFER),hl
    xor a
    ld (kdIOStarted),a
    ld a,(kdOperation)
    ld hl,KD_OBJECT
    call fileHandle
    push af
    xor a
    ld (kdFarActive),a
    pop af
    push af
    pop hl
    ld (kdResult),hl
    ld a,(kdIOStarted)
    or a
    ld hl,0
    jp z,kdHandleCount
    ld hl,(KD_OBJECT+FH_COUNT)
kdHandleCount:
    ld (kdActual),hl
    ld b,h
    ld c,l
kdHandleAdvance:
    ld hl,(kdActual)
    ex de,hl
    ld hl,(kdTotal)
    add hl,de
    ld (kdTotal),hl
    ld hl,(kdBuffer)
    add hl,de
    ld (kdBuffer),hl
    ld hl,(kdLeft)
    call sub_hl_de
    ld (kdLeft),hl
    ld a,h
    or l
    jp z,kdHandleDone
    ld a,(kdResult)
    and 1
    jp nz,kdHandleDone
    ld hl,(kdChunk)
    ex de,hl
    ld hl,(kdActual)
    call cmp_hl_de
    jp z,kdHandleLoop
kdHandleDone:
    ld hl,(kdOriginalBuffer)
    ld (KD_OBJECT+FH_BUFFER),hl
    ld hl,(kdTotal)
    ld (KD_OBJECT+FH_COUNT),hl
kdHandleReturn:
    ld hl,(kdClientObject)
    ex de,hl
    ld hl,KD_OBJECT
    ld bc,FILE_HANDLE
    call kdStore
    jp c,kdBad
    ld hl,(kdResult)
    ld (kdRegs+6),hl
kdRestorePointer:
    ld hl,(kdClientObject)
    ld (kdRegs),hl
kdFinish:
    xor a
    ld (kdBusy),a
    ld (BANK_FRAME_STATUS),a
    jp kdLoadRegs
kdUnsupported:
    xor a
    ld (kdBusy),a
    jp dosUnsupported
kdBad:
    call blockArgument
kdError:
    push af
    pop hl
    ld (kdRegs+6),hl
    jp kdFinish
kdReentry:
    pop af
    ld a,DOS_ERR_ACCESS
    scf
    ret

; Загружаем регистры и передаём управление функции с обычным RET.
kdInvoke:
    ld hl,(kdEntry)
    push hl
kdLoadRegs:
    ld hl,(kdRegs+6)
    push hl
    ld hl,(kdRegs+4)
    ld b,h
    ld c,l
    ld hl,(kdRegs+2)
    ex de,hl
    ld hl,(kdRegs)
    pop af
    ret
kdCapture:
    ld (kdRegs),hl
    ex de,hl
    ld (kdRegs+2),hl
    ld h,b
    ld l,c
    ld (kdRegs+4),hl
    push af
    pop hl
    ld (kdRegs+6),hl
    ret

; HL/BC — внешний диапазон, сохранены; CF=ошибка. F800h — начало хвоста.
kdRange:
    push hl
    add hl,bc
    jp c,kdRangeFail
    ld a,(kdPage)
    or a
    ld a,h
    jp nz,kdRangeBank
    cp 0FCh
    jp kdRangeCompare
kdRangeBank:
    cp 0F8h
kdRangeCompare:
    jp c,kdRangeOk
    jp nz,kdRangeFail
    ld a,l
    or a
    jp nz,kdRangeFail
kdRangeOk:
    pop hl
    or a
    ret
kdRangeFail:
    pop hl
    scf
    ret
; File objects/data must not overwrite the caller's MEM tail or context.
; Driver registration uses kdRange separately to read installed service code.
kdFileRange:
    push hl
    add hl,bc
    jp c,kdRangeFail
    push de
    ld de,IO_BANK_KEYS
    call cmp_hl_de
    pop de
    jp c,kdRangeOk
    jp z,kdRangeOk
    jp kdRangeFail

;----------------------------------------------------------------------------
; MXOS
; Получить путь ASCIIZ (kdFetchPath) или строку FILE_EXEC (kdFetchLine).
; вход: HL = адрес в странице kdPage. Буфер назначения KD_PATH, до MAX_PATH.
; выход: CF=0, если скопирован терминатор (соответственно 0 или байт <20h).
; При CF=1 строка не завершена в допустимом диапазоне. AF/BC/DE/HL рабочие.
;----------------------------------------------------------------------------
kdFetchPath:
    ld c,1
    jp kdFetchString
kdFetchLine:
    ld c,020h
kdFetchString:
    ld de,KD_PATH
    ld b,MAX_PATH
;----------------------------------------------------------------------------
; MXOS
; Прочитать строку через общее окно, ограничив запрос доступной памятью.
; вход: HL = адрес в странице kdPage, DE = локальный буфер, B = ёмкость
; (до MAX_PATH), C = порог терминатора. Любой байт <C завершает строку.
; выход: CF=0 при завершении, HL/DE после терминатора, B = остаток ёмкости.
; CF=1 при выходе за границу или исчерпании ёмкости. AF/BC/DE/HL рабочие.
;----------------------------------------------------------------------------
kdReadString:
    push de
    ex de,hl
    ld hl,0FC00h
    ld a,(kdPage)
    or a
    jp z,kdReadLimit
    ld h,0F8h
kdReadLimit:
    call sub_hl_de
    jp c,kdRangeFail
    ld a,h
    or a
    jp nz,kdReadBounded
    ld a,l
    cp b
    jp nc,kdReadBounded
    ld b,a
kdReadBounded:
    ex de,hl
    pop de
    ld a,b
    or a
    jp z,kdReadMissing
;----------------------------------------------------------------------------
; MXOS
; Получить проверенный диапазон или строку порциями не более 32 байт.
; вход: HL = адрес клиента, DE = локальный приёмник, B = длина 1..128,
; C = 0 для двоичного диапазона или порог терминатора для строки.
; выход: CF=0 при успехе, HL/DE = следующие адреса, B = остаток длины.
; CF=1 при исчерпании длины строки без терминатора. AF/BC/DE/HL рабочие.
;----------------------------------------------------------------------------
kdReadChunks:
    push bc
    push de
    ld a,b
    cp 32
    jp c,kdReadChunk
    ld b,32
kdReadChunk:
    ld a,(kdPage)
    call memDosReadChunk
    ld a,e
    pop de
    push af
    push hl
    sub BANK_WINDOW & 0FFh
    ld c,a
    ld b,0
    push bc
    ld hl,BANK_WINDOW
    call moveBytesForward
    ld d,b
    ld e,c
    pop bc
    pop hl
    pop af
    ld a,c
    cpl
    inc a
    pop bc
    jp c,kdReadTerminated
    add a,b
    ld b,a
    jp nz,kdReadChunks
    ld a,c
    or a
    ret z
kdReadMissing:
    scf
    ret
kdReadTerminated:
    add a,b
    ld b,a
    or a
    ret

; HL=источник клиента, DE=локальный приёмник, BC=длина.
kdFetch:
    call kdRange
    ret c
    ld a,(kdPage)
    cp DOS_IO_PAGE
    jp z,kdFetchFar
    ld a,b
    or a
    jp nz,kdFetchFar
    ld a,c
    or a
    ret z
    ; Два полных блока выгоднее провести развёрткой CPM32.
    cp 64
    jp nc,kdFetchFar
    push bc
    ld b,c
    ld c,0
    call kdReadChunks
    pop bc
    ret
kdFetchFar:
    ld (kdCopy+1),hl
    ex de,hl
    ld (kdCopy+4),hl
    ld a,(kdPage)
    ld (kdCopy),a
    ld a,DOS_IO_PAGE
    ld (kdCopy+3),a
    jp kdCopyCount
; HL=локальный источник, DE=приёмник клиента, BC=длина.
kdStore:
    ex de,hl
    call kdRange
    ret c
    ld a,(kdPage)
    cp DOS_IO_PAGE
    jp z,kdStoreFar
    ld a,b
    or a
    jp nz,kdStoreFar
    ld a,c
    or a
    ret z
    ; Два полных блока выгоднее провести развёрткой CPM32.
    cp 64
    jp nc,kdStoreFar
    ex de,hl
    push bc
    ld b,c
    call kdWriteChunks
    pop bc
    ret
kdStoreFar:
    ld (kdCopy+4),hl
    ex de,hl
    ld (kdCopy+1),hl
    ld a,(kdPage)
    ld (kdCopy+3),a
    ld a,DOS_IO_PAGE
    ld (kdCopy),a
kdCopyCount:
    ld h,b
    ld l,c
    ld (kdCopy+6),hl
    ld hl,kdCopy
    jp bankCopyFar

;----------------------------------------------------------------------------
; MXOS
; Передать двоичный диапазон порциями не более 32 байт.
; вход: HL = локальный источник, DE = проверенный приёмник клиента,
; B = длина 1..63, kdPage = страница клиента.
; выход: CF=0, HL/DE = следующие адреса. AF/BC рабочие.
;----------------------------------------------------------------------------
kdWriteChunks:
    push bc
    push de
    ld a,b
    cp 32
    jp c,kdWriteChunk
    ld a,32
kdWriteChunk:
    ld c,a
    ld b,0
    ld de,BANK_WINDOW
    call moveBytesForward
    ld a,c
    sub BANK_WINDOW & 0FFh
    ld b,a
    ld c,0
    pop de
    push hl
    ld a,(kdPage)
    call memDosWriteChunk
    ld a,l
    sub BANK_WINDOW & 0FFh
    cpl
    inc a
    pop hl
    pop bc
    add a,b
    ld b,a
    jp nz,kdWriteChunks
    xor a
    ret

; Внешний запрос может вызвать только адаптер NEAR или скалярную службу.
; Бит 15 отмечает скалярный вход: kdInvokeScalar загружает MX_REGS
; и завершает запрос через kdFinish. Пропуски возвращают UNSUPPORTED.
    MACRO KD_EXTERNAL operation, handler, scalar
        ASSERT handler < 08000h
        DUP (operation & 0FFh)-(($-kdExternalTable)/2)
            DW kdUnsupported
        EDUP
        DW handler+scalar*08000h
    ENDM
kdExternalTable:
    KD_EXTERNAL MXOS_DOS_FILE_LIST,             kdFileList,         0
    KD_EXTERNAL MXOS_DOS_FILE_GET_SET_DRIVE,    fileGetSetDrive,    1
    KD_EXTERNAL MXOS_DOS_FILE_DELETE,           kmDelete,           0
    KD_EXTERNAL MXOS_DOS_FILE_RENAME,           kmRename,           0
    KD_EXTERNAL MXOS_DOS_FILE_LOAD_INFO,        kmInfo,             0
    KD_EXTERNAL MXOS_DOS_FILE_GET_SET_ATTR,     kmAttr,             0
    KD_EXTERNAL MXOS_DOS_FILE_NAME_PREPARE,     kmName,             0
    KD_EXTERNAL MXOS_DOS_FILE_EXEC,             keRequest,          0
    KD_EXTERNAL MXOS_DOS_GET_DRIVE_FREE_SPACE,  getDriveFreeSpace,  1
    KD_EXTERNAL MXOS_DOS_MOUNT_VOLUME,          mountVolume,        1
    KD_EXTERNAL MXOS_DOS_FLUSH_VOLUME,          flushVolume,        1
    KD_EXTERNAL MXOS_DOS_FILE_SELECT_DRIVE,     fileChangeDrive,    1
    KD_EXTERNAL MXOS_DOS_FILE_COPY,             kcCopy,             0
    KD_EXTERNAL MXOS_DOS_FILE_GET_DRIVES,       kmDrives,           0
    KD_EXTERNAL MXOS_DOS_GET_ERROR,             dosGetError,        1
    KD_EXTERNAL MXOS_DOS_SECTOR_IO,             krSector,           0
    KD_EXTERNAL MXOS_DOS_TIMER,                 timerService,       1
    KD_EXTERNAL MXOS_DOS_FILEHANDLE,            kdHandle,           0
    KD_EXTERNAL MXOS_DOS_DIRECTORY,             kdDirectory,        0
    KD_EXTERNAL MXOS_DOS_CLI,                   kdCli,              0
    KD_EXTERNAL MXOS_DOS_SET_SHELL,             setShell,           1
    KD_EXTERNAL MXOS_DOS_RAW_IO,                krRaw,              0
    KD_EXTERNAL MXOS_DOS_DRIVER_REFRESH,        dosDriverRefresh,   1
    KD_EXTERNAL MXOS_DOS_FONT_CONFIG,           kdFontConfig,       0
kdExternalEnd:
    ASSERT (kdExternalEnd-kdExternalTable)/2 <= 128
kdLocalTable:
    INCLUDE "../../bin/banked/kernel-services.inc"
kdLocalEnd:
kdRegs           = DOS_REQUEST_SCRATCH+0
kdEntry          = DOS_REQUEST_SCRATCH+8
kdClientObject   = DOS_REQUEST_SCRATCH+10
kdBuffer         = DOS_REQUEST_SCRATCH+12
kdOriginalBuffer = DOS_REQUEST_SCRATCH+14
kdLeft           = DOS_REQUEST_SCRATCH+16
kdChunk          = DOS_REQUEST_SCRATCH+18
kdActual         = DOS_REQUEST_SCRATCH+20
kdTotal          = DOS_REQUEST_SCRATCH+22
kdResult         = DOS_REQUEST_SCRATCH+24
kdCopy           = DOS_REQUEST_SCRATCH+26
    ASSERT DOS_REQUEST_SCRATCH+34 <= DOS_ARENA_START


; Caller submits one stream. Only the listed native operations share KD_DATA
; safely. Each optional inline HL is local to this batch in DOS.
dosCommandBatch:
    ld hl,(kdRegs+4)
    dec hl
    ld a,h
    or a
    jp nz,kdBad
    inc hl
    ld b,h
    ld c,l
    ld de,KD_DATA
    add hl,de
    push hl
    ld hl,(kdRegs)
    call kdFetch
    pop de
    jp c,kdBad
    ld hl,KD_DATA
dosBatchNext:
    call cmp_hl_de
    jp nc,dosBatchDone
    ld a,(hl)
    or a
    jp z,dosBatchDone
    ld bc,fileChangeDrive
    cp MXOS_DOS_FILE_SELECT_DRIVE & 255
    jp z,dosBatchFound
    ld bc,flushVolume
    cp MXOS_DOS_FLUSH_VOLUME & 255
    jp z,dosBatchFound
    ld bc,fileDelete
    cp MXOS_DOS_FILE_DELETE & 255
    jp nz,dosBatchUnsupported
dosBatchFound:
    push de
    inc hl
    ld e,(hl)
    ld d,0
    dec hl
    push hl
    add hl,de
    ex (sp),hl
    inc hl
    inc hl
    push bc
    DEFINE BATCH_REGS kdRegs
    INCLUDE "../common/batch-args.inc"
    UNDEFINE BATCH_REGS
    pop hl
    ld (kdEntry),hl
    call kdInvoke
    pop hl
    pop de
    jp nc,dosBatchNext
    call dosIoError
    jp kdError
dosBatchUnsupported:
    ld a,DOS_ERR_UNSUPPORTED
    scf
    jp kdError
dosBatchDone:
    xor a
    jp kdError
