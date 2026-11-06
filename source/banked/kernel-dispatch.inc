; Внешний DOS-запрос: A=страница клиента, BC=группа/функция,
; DE=MX_REGS в общем окне. Результат AF/BC/DE/HL, SP сохраняет транспорт.
; NEAR-объекты принадлежат клиенту, локальные копии — только банку DOS.
; Повторный вход не разрешает вторую файловую транзакцию.
KD_OBJECT = DOS_BANK_OBJECT_ADDR
KD_PATH = DOS_BANK_PATH_ADDR
KD_DATA = DOS_BANK_DATA_ADDR
KD_CHUNK = DOS_BANK_DATA_SIZE
kernelDispatch:
    ; Межстраничный пакет находится в общем окне, даже для инициатора DOS.
    ld de,BANK_FRAME_REGS
    jp kdEntryCommon
kernelLocalDispatch:
    ; DE уже указывает на локальный MX_REGS: транспорт/копировщик не нужны.
    ld a,DOS_IO_PAGE
    ld (BANK_FRAME_STATUS),a
kdEntryCommon:
    push af
    ld a,(kdBusy)
    or a
    jp nz,kdReentry
    pop af
    ld (kdPage),a
    ld a,b
    cp MX_GROUP_DOS
    jp nz,dosUnsupported
    ld a,c
    ld (kdFunction),a
    ; Подготовленный образ действителен до следующего запроса DOS.
    ; Чужой FILELOAD не может использовать курсор владельца образа.
    cp MX_DOS_FILELOAD & 0FFh
    jp z,kdImageContextReady
    push af
    xor a
    ld (kiReady),a
    pop af
kdImageContextReady:
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
    ld a,(kdPage)
    cp DOS_IO_PAGE
    jp z,kdLocal
    ld a,(kdFunction)
    cp MX_DOS_FILECREATE & 0FFh
    jp z,krCreate
    cp MX_DOS_FILECREATEPREPARE & 0FFh
    jp z,krPrepare
    cp MX_DOS_SECTORIO & 0FFh
    jp z,krSector
    cp MX_DOS_RAWIO & 0FFh
    jp z,krRaw
    cp MX_DOS_FILEEXEC & 0FFh
    jp z,keRequest
    cp MX_DOS_FILECOPY & 0FFh
    jp z,kcCopy
    cp MX_DOS_IMAGEOPEN & 0FFh
    jp z,kiOpen
    cp MX_DOS_FILELOAD & 0FFh
    jp z,kiLoad
    cp MX_DOS_FILELOAD2 & 0FFh
    jp z,kiRaw
    cp MX_DOS_FILEGETDRIVES & 0FFh
    jp z,kmDrives
    cp MX_DOS_FILENAMEPREPARE & 0FFh
    jp z,kmName
    cp MX_DOS_FILELOADINFO & 0FFh
    jp z,kmInfo
    cp MX_DOS_FILEGETSETATTR & 0FFh
    jp z,kmAttr
    cp MX_DOS_FILEDELETE & 0FFh
    jp z,kmDelete
    cp MX_DOS_FILEDELETEONDRIVE & 0FFh
    jp z,kmDelete
    cp MX_DOS_FILERENAME & 0FFh
    jp z,kmRename
    cp MX_DOS_FILERENAMEONDRIVE & 0FFh
    jp z,kmRename
    cp MX_DOS_FILESTREAM & 0FFh
    jp z,kfStream
    cp MX_DOS_FILELIST & 0FFh
    jp z,kdFileList
    cp MX_DOS_FILEHANDLE & 0FFh
    jp z,kdHandle
    cp MX_DOS_DIRECTORY & 0FFh
    jp z,kdDirectory
    cp MX_DOS_CLI & 0FFh
    jp z,kdCli
    cp MX_DOS_FILELOADPATH & 0FFh
    jp z,kdPathIO
    cp MX_DOS_FILESAVEPATH & 0FFh
    jp z,kdPathIO
    ; Разрешены только функции, не возвращающие локальных указателей.
    ld de,kdScalars
kdScalarFind:
    ld a,(de)
    inc de
    cp 0FFh
    jp z,kdUnsupported
    ld b,a
    ld a,(kdFunction)
    cp b
    jp z,kdScalarFound
    inc de
    inc de
    jp kdScalarFind
kdScalarFound:
    ld a,(de)
    ld l,a
    inc de
    ld a,(de)
    ld h,a
    ld (kdEntry),hl
    call kdInvoke
    call kdCapture
    jp kdFinish
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
    ld (kdEntry),hl
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
    ld de,KD_PATH
    ld bc,MAX_PATH
    call kdFetch
    jp c,kdBad
    ; Проверяем завершение до вызова парсера пути.
    ld hl,KD_PATH
    ld b,MAX_PATH
kdPathScan:
    ld a,(hl)
    or a
    jp z,kdPathReady
    inc hl
    dec b
    jp nz,kdPathScan
    jp kdBad
kdPathReady:
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
    ld b,MAX_PATH
    xor a
kdCwdClear:
    ld (hl),a
    inc hl
    dec b
    jp nz,kdCwdClear
    call kdInvoke
    call kdCapture
    ld a,(kdRegs+6)
    and 1
    jp nz,kdRestorePointer
    ld hl,(kdClientObject)
    ex de,hl
    ld hl,KD_PATH
    ld bc,MAX_PATH
    call kdStore
    jp c,kdBad
    jp kdRestorePointer
kdDirectoryScalar:
    call kdInvoke
    call kdCapture
    jp kdFinish

; Текстовые команды передаются как MAX_PATH. Наблюдатель вызывается в
; странице клиента; подтверждения COPY/MOVE требуют отдельного адаптера.
kdCli:
    ld hl,cliService
    ld (kdEntry),hl
    ld a,(kdRegs+7)
    cp CLI_OP_COPY
    jp z,kcCopy
    cp CLI_OP_MOVE
    jp z,kcMove
    cp CLI_OP_OBSERVER
    jp z,koRegister
    cp CLI_OP_QUERY
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
    ld a,(kdRegs+7)
    cp FILE_OP_CLOSE+1
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
    ld hl,KD_OBJECT
    call fileHandle
    push af
    pop hl
    ld (kdResult),hl
    jp kdHandleReturn
kdHandleIO:
    ld hl,(KD_OBJECT+FH_BUFFER)
    ld (kdOriginalBuffer),hl
    ld (kdBuffer),hl
    ld hl,(KD_OBJECT+FH_COUNT)
    ld (kdLeft),hl
    ld b,h
    ld c,l
    ld hl,(kdBuffer)
    call kdRange
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
    ld hl,(kdLeft)
    ld a,h
    cp KD_CHUNK/256
    jp c,kdHaveChunk
    ld hl,KD_CHUNK
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
; HL=источник клиента, DE=локальный приёмник, BC=длина.
kdFetch:
    call kdRange
    ret c
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

kdScalars:
    DB MX_DOS_DRIVERSELECT & 0FFh
    DW dosDriverSelect
    DB MX_DOS_GETDRIVEFREESPACE & 0FFh
    DW getDriveFreeSpace
    DB MX_DOS_MOUNTVOLUME & 0FFh
    DW mountVolume
    DB MX_DOS_FILEGETSETDRIVE & 0FFh
    DW fileGetSetDrive
    DB MX_DOS_FILESELECTDRIVE & 0FFh
    DW fileChangeDrive
    DB MX_DOS_GETERROR & 0FFh
    DW dosGetError
    DB MX_DOS_FLUSHVOLUME & 0FFh
    DW flushVolume
    DB MX_DOS_SETSHELL & 0FFh
    DW setShell
    DB 0FFh
kdLocalTable:
    INCLUDE "../../bin/banked/kernel-services.inc"
kdLocalEnd:
kdBusy: DB 0
kdPage: DB 0
kdFunction: DB 0
kdOperation: DB 0
kdIOStarted: DB 0
kdRegs: DS MX_REGS,0
kdEntry: DW 0
kdClientObject: DW 0
kdBuffer: DW 0
kdOriginalBuffer: DW 0
kdLeft: DW 0
kdChunk: DW 0
kdActual: DW 0
kdTotal: DW 0
kdResult: DW 0
kdCopy: DS 8,0
