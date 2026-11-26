; Временные MAIN-вызовы получают собственные теги, не совпадающие с банками
; и тегами модулей. Кадры арены сохраняются до возврата вложенной программы.
; appEnter/appLeave сохраняют BC/DE/HL; AF — результат (Enter) или сохранён.
appEnter:
    ld a,(appDepth)
    cp APP_MAX_DEPTH
    jp nc,appOverflow
    push bc
    push de
    push hl
    call appHeapAddress
    xor a
    ld (hl),a
    inc hl
    ld (hl),a                ; Нулевой конец — у нового владельца ещё нет кучи.
    ld a,(appDepth)
    call appFrame
    ld a,(moduleActor)
    ld (hl),a
    inc hl
    ld a,(appOwner)
    ld (hl),a
    inc hl
    ld a,(memoryMainOwner)
    ld (hl),a
    inc hl
    ex de,hl
    ld hl,(memoryMainStart)
    call appSaveWord
    ld hl,(memoryMainEnd)
    call appSaveWord
    ld hl,(appImage)
    call appSaveWord
    xor a
    ld (moduleActor),a
    ld hl,0
    ld (memoryMainStart),hl
    ld (memoryMainEnd),hl
    ld (appImage),hl
    ld a,(appDepth)
    add a,APP_OWNER_BASE
    ld (appOwner),a
    ld a,(appDepth)
    inc a
    ld (appDepth),a
    pop hl
    pop de
    pop bc
    xor a
    ret
appOverflow:
    ld a,MEM_ERROR_STACK
    scf
    ret

;----------------------------------------------------------------------------
; MXOS
;
; Дописать слово в кадр сохранения состояния приложения.
;
; вход:
;   hl = значение, de = адрес двух свободных байтов.
;
; выход:
;   Слово записано от младшего байта к старшему, de продвинут на два, a = h. bc, hl и
;   флаги сохранены.
;----------------------------------------------------------------------------

appSaveWord:
    ld a,l
    ld (de),a
    inc de
    ld a,h
    ld (de),a
    inc de
    ret
appFrame:
    ld l,a
    ld h,0
    add hl,hl
    add hl,hl
    add hl,hl
    ld e,a
    ld d,0
    add hl,de              ; 9 bytes per saved application
    ld de,appFrames
    add hl,de
    ret
appLeave:
    push af
    push bc
    push de
    push hl
    call klReclaim
    ld a,(appOwner)
    ld c,a
    call appReleaseOwner
    ld a,(appDepth)
    dec a
    ld (appDepth),a
    call appFrame
    ld a,(hl)
    ld (moduleActor),a
    inc hl
    ld a,(hl)
    ld (appOwner),a
    inc hl
    ld a,(hl)
    ld (memoryMainOwner),a
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    push hl
    ex de,hl
    ld (memoryMainStart),hl
    pop hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    push hl
    ex de,hl
    ld (memoryMainEnd),hl
    pop hl
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld (appImage),hl
    call memoryPublishTop
    pop hl
    pop de
    pop bc
    pop af
    ret

; C=владелец. Освобождаем дочерние модули и обычные выделения.
appReleaseOwner:
    call timerReleaseOwner
    call fileopsReleaseOwner
    ld hl,MODULE_TABLE
    ld b,MODULE_MAX
appReleaseModule:
    ld a,(hl)
    inc hl
    or (hl)
    dec hl
    jp z,appReleaseNext
    push hl
    ld de,8
    add hl,de
    ld a,(hl)
    pop hl
    cp c
    jp nz,appReleaseNext
    push bc
    push hl
    call mdRelease
    pop hl
    pop bc
appReleaseNext:
    ld de,MODULE_RECORD_SIZE
    add hl,de
    dec b
    jp nz,appReleaseModule
    ld hl,MEM_ALLOC_TABLE
    ld b,MEM_ALLOC_RECORDS
appReleaseBlock:
    push hl
    ld de,5
    add hl,de
    ld a,(hl)
    pop hl
    cp c
    jp nz,appReleaseBlockNext
    ld (hl),0FFh
appReleaseBlockNext:
    ld de,6
    add hl,de
    dec b
    jp nz,appReleaseBlock
    ret

; Указатель на ключ резидента по записи модуля. HL/BC сохранены,
; DE=адрес ключа, Z=ключ нулевой; AF разрушен.
residentKey:
    push hl
    ld de,9
    add hl,de
    ld a,(hl)
    sub MODULE_OWNER_BASE
    add a,a
    ld e,a
    ld d,0
    ld hl,residentKeys
    add hl,de
    ex de,hl
    ld a,(de)
    ld l,a
    inc de
    ld a,(de)
    dec de
    or l
    pop hl
    ret

; RESIDENT: A=0 сохраняет текущий BANK-модуль с ненулевым ключом DE;
; A=1 ищет ключ DE; A=2 перечисляет с индекса B (0..7).
; Результат HL=handle (0 — нет), B=следующий индекс, C=банк, DE=ключ.
; Резидент становится самостоятельным сервисом и доступен другим владельцам.
residentService:
    ld a,(kdRegs+7)
    cp RESIDENT_SHELL_STATE
    jp z,residentShellState
    cp RESIDENT_POLICY
    jp nc,pressureService
    cp RESIDENT_SHELL_SET
    jp z,residentShellSet
    cp RESIDENT_SHELL_RESUME
    jp z,residentShellResume
    cp RESIDENT_DIRECTORY_EPOCH
    jp z,residentDirectoryEpoch
    cp RESIDENT_RELEASE
    jp z,residentRelease
    cp RESIDENT_RETAIN
    jp z,residentRetain
    cp RESIDENT_FIND
    jp z,residentFind
    cp RESIDENT_ENUM
    jp nz,kdBad
    ld a,(kdRegs+5)
    cp MODULE_MAX
    jp nc,residentNone
    jp residentScanStart
; The shell owns its entry protocol. DOS stores a key, never a persistent PC/SP.
residentShellSet:
    ld hl,(kdRegs+2)
    ld (residentShellKey),hl
    xor a
    jp kdError

;----------------------------------------------------------------------------
; MXOS
; Скопировать двоичный снимок панелей между NC и банком DOS.
; вход: текущий модуль владеет ключом NC_BANK_KEY; B=0 чтение или 1 запись,
; HL=адрес в EXEC, DE=RESIDENT_STATE_BYTES. Файловый ввод-вывод отсутствует.
; выход: CF/A=статус. Пустой слот при чтении возвращает NOT_FOUND; снимок
; доступен до холодной загрузки DOS. BC/DE/HL рабочие.
;----------------------------------------------------------------------------
residentShellState:
    call pressureNcActor
    jp nz,residentDenied
    ld hl,(kdRegs+2)
    ld de,RESIDENT_STATE_BYTES
    call cmp_hl_de
    jp nz,kdBad
    ld hl,(kdRegs)
    ld bc,RESIDENT_STATE_BYTES
    call kdFileRange
    jp c,kdBad
    ld a,(kdRegs+5)
    or a
    jp z,residentShellRead
    dec a
    jp nz,kdBad
    ld de,pressureState
    call kdFetch
    jp c,kdBad
    ld a,1
    ld (pressureStateSize),a
    xor a
    jp kdError
residentShellRead:
    ld a,(pressureStateSize)
    or a
    ld a,DOS_ERR_NOT_FOUND
    jp z,mdError
    ex de,hl
    ld hl,pressureState
    call kdStore
    jp kdError
residentDirectoryEpoch:
    ld a,(kdRegs+5)
    cp RESIDENT_EPOCH_ALL
    jp z,residentDriveEpochs
    or a
    jp nz,kdBad
    ld hl,(cliChangeEpoch+2)
    ex de,hl
    ld hl,(cliChangeEpoch)
    xor a
    call kdCapture
    jp kdFinish
residentDriveEpochs:
    ld hl,(kdRegs)
    ld bc,32
    call kdFileRange
    jp c,kdBad
    ex de,hl
    ld hl,cliDriveEpochs
    call kdStore
    jp c,kdBad
    xor a
    jp kdError
residentRelease:
    ld a,(moduleActor)
    sub MODULE_OWNER_BASE
    cp MODULE_MAX
    jp nc,residentDenied
    ld hl,residentPriorities
    call pressureByte
    ld c,a
    ld a,(hl)
    cp RESIDENT_PRIORITY_PINNED
    jp z,mdUnloadProtected
    ld a,c
    ld b,a
    ld l,a
    ld h,0
    add hl,hl
    ld de,residentKeys
    add hl,de
    ld (hl),0
    inc hl
    ld (hl),0
    ld hl,MODULE_TABLE+8
    ld de,MODULE_RECORD_SIZE
    ld a,b
    or a
    jp z,residentReleaseOwner
residentReleaseRecord:
    add hl,de
    dec b
    jp nz,residentReleaseRecord
residentReleaseOwner:
    ld (hl),DOS_IO_PAGE ; The system dispatcher releases it after the call.
    xor a
    jp kdError
residentShellResume:
    ld hl,(residentShellKey)
    ld a,h
    or l
    jp z,residentShellMissing
    ex de,hl
    ld hl,residentKeys
    ld b,MODULE_MAX
residentShellScan:
    ld a,(hl)
    inc hl
    cp e
    jp nz,residentShellNext
    ld a,(hl)
    cp d
    jp z,residentShellFound
residentShellNext:
    inc hl
    dec b
    jp nz,residentShellScan
residentShellMissing:
    ld hl,(residentShellKey)
    ld (kdRegs),hl
    xor a
    scf
    jp kdError
residentShellFound:
    ld a,MODULE_MAX
    sub b
    ld b,a
    ld hl,MODULE_TABLE
    ld de,MODULE_RECORD_SIZE
    or a
    jp z,residentShellRecord
residentShellRecordNext:
    add hl,de
    dec b
    jp nz,residentShellRecordNext
residentShellRecord:
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld (residentShellHandle),hl
    ld a,(kdPage)
    push af
    ld hl,(residentShellHandle)
    push hl
    ld hl,0100h ; AF: A=RESTORE, flags clear.
    push hl
    ld hl,0
    push hl
    push hl
    push hl
    add hl,sp
    ex de,hl
    ld hl,(residentShellHandle)
    xor a
    ld (kdBusy),a
    MXOS_CALL_REGS MXOS_DOS_MODULE_CALL
    jp c,residentShellStatusReady
    ld hl,6
    add hl,sp
    ld e,(hl)
    inc hl
    ld d,(hl)
    push de
    pop af
residentShellStatusReady:
    push af
    pop bc
    ld hl,8
    add hl,sp
    ld sp,hl
    pop hl
    push bc
    ld a,1
    MXOS_CALL_REGS MXOS_DOS_MODULE_UNLOAD
    pop bc
    pop af
    ld (kdPage),a
    push bc
    pop af
    call nc,pressureRunCommand
    call kdCapture
    jp kdFinish
residentFind:
    xor a
residentScanStart:
    ld (residentIndex),a
residentScan:
    ld a,(residentIndex)
    cp MODULE_MAX
    jp nc,residentNone
    ld l,a
    ld h,0
    add hl,hl
    ld de,residentKeys
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    ld a,d
    or e
    jp z,residentNext
    ld a,(kdRegs+7)
    cp RESIDENT_ENUM
    jp z,residentResult
    ld hl,(kdRegs+2)
    call cmp_hl_de
    jp z,residentResult
residentNext:
    ld a,(residentIndex)
    inc a
    ld (residentIndex),a
    jp residentScan
residentResult:
    push de
    ld hl,MODULE_TABLE
    ld de,MODULE_RECORD_SIZE
    ld a,(residentIndex)
    ld b,a
    or a
    jp z,residentRecordReady
residentRecordLoop:
    add hl,de
    dec a
    jp nz,residentRecordLoop
residentRecordReady:
    inc b
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    inc hl
    ld c,(hl)
    ex de,hl
    pop de
    xor a
    call kdCapture
    jp kdFinish
residentNone:
    ld hl,0
    ld de,0
    ld bc,MODULE_MAX << 8
    xor a
    call kdCapture
    jp kdFinish
residentRetain:
    ld a,(moduleActor)
    sub MODULE_OWNER_BASE
    cp MODULE_MAX
    jp nc,residentDenied
    ld (residentIndex),a
    ld hl,(kdRegs+2)
    ld a,h
    or l
    jp z,kdBad
    ; MAIN-память не сохраняется за резидентом: её заменит следующая программа.
    ld a,(memoryMainOwner)
    ld b,a
    ld a,(moduleActor)
    cp b
    jp nz,residentCheckMainBlocks
    ld hl,(memoryMainStart)
    ld a,h
    or l
    jp nz,residentDenied
residentCheckMainBlocks:
    ld hl,MEM_ALLOC_TABLE
    ld b,MEM_ALLOC_RECORDS
residentMainBlock:
    ld a,(hl)
    or a
    jp nz,residentMainNext
    push hl
    ld de,5
    add hl,de
    ld a,(moduleActor)
    cp (hl)
    pop hl
    jp z,residentDenied
residentMainNext:
    ld de,6
    add hl,de
    dec b
    jp nz,residentMainBlock
    ; Регистрироваться нужно до загрузки дочерних модулей: их MAIN-ресурсы
    ; нельзя безопасно превратить в сохраняемые задним числом.
    ld hl,MODULE_TABLE
    ld b,MODULE_MAX
residentChildren:
    ld a,(hl)
    inc hl
    or (hl)
    dec hl
    jp z,residentChildNext
    push hl
    ld de,8
    add hl,de
    ld a,(moduleActor)
    cp (hl)
    pop hl
    jp z,residentDenied
residentChildNext:
    ld de,MODULE_RECORD_SIZE
    add hl,de
    dec b
    jp nz,residentChildren
    ld hl,(kdRegs+2)
    ; Ключ уникален среди живых резидентов.
    ex de,hl
    ld hl,residentKeys
    ld b,MODULE_MAX
residentUnique:
    ld a,(hl)
    inc hl
    cp e
    jp nz,residentUniqueNext
    ld a,(hl)
    cp d
    jp z,residentDenied
residentUniqueNext:
    inc hl
    dec b
    jp nz,residentUnique
    ld hl,MODULE_TABLE
    ld de,MODULE_RECORD_SIZE
    ld a,(residentIndex)
    or a
    jp z,residentRetainRecord
residentRetainLoop:
    add hl,de
    dec a
    jp nz,residentRetainLoop
residentRetainRecord:
    push hl
    ld de,3
    add hl,de
    ld a,(hl)
    or a
    pop hl
    jp z,kdBad             ; MAIN-резидент не защищён от следующей загрузки.
    call residentKey
    push hl
    ld hl,(kdRegs+2)
    call appSaveWord
    pop hl
    ld de,8
    add hl,de
    ld (hl),0             ; Самостоятельный сервис, не потомок приложения.
    xor a
    jp kdError
residentDenied:
    ld a,MEM_ERROR_OWNER
    jp mdError

appFrames = DOS_APP_FRAMES
    ASSERT appFrames+APP_MAX_DEPTH*9 <= DOS_CACHE_BOOT_ADDR

 ; Z=текущий модуль и его предки не резиденты. BC/DE/HL сохранены.
residentActorCheck:
    push bc
    push de
    push hl
    ld a,(moduleActor)
    ld b,MODULE_MAX
residentAncestor:
    sub MODULE_OWNER_BASE
    cp MODULE_MAX
    jp nc,residentTransient
    ld c,a
    add a,a
    ld e,a
    ld d,0
    ld hl,residentKeys
    add hl,de
    ld a,(hl)
    inc hl
    or (hl)
    jp nz,residentAncestorDone
    ld hl,MODULE_TABLE+8
    ld de,MODULE_RECORD_SIZE
    ld a,c
    or a
    jp z,residentParentReady
residentParentLoop:
    add hl,de
    dec a
    jp nz,residentParentLoop
residentParentReady:
    ld a,(hl)
    dec b
    jp nz,residentAncestor
residentTransient:
    xor a
residentAncestorDone:
    pop hl
    pop de
    pop bc
    ret
