; EXEC profile runtime. Rendering is queued; only flush/key calls enter MAIN.
;----------------------------------------------------------------------------
; MXOS
;
; Установить или возобновить резидентную оболочку NC_CORE.DLL в EXEC.
;
; вход:
;   a = 2 для закрытия, 1 для восстановления либо буква диска при установке. При b != 0
;   выполняется регистрация, при b = 0 запускается интерфейс.
;
; выход:
;   При штатном завершении a = 0, cf = 0; при ошибке a = код, cf = 1. Графика
;   выполняется через UI_GFX в MAIN.
;----------------------------------------------------------------------------

ncBankEntry:
    cp 2 ; Shared resident convention: RESTORE=1, CLOSE=2.
    jp z,ncBankClose
    ld c,a
    ld a,b
    ld (ncBankInitOnly),a
    ld a,(ncBankRegistered)
    or a
    jp nz,ncBankEntryDriveReady
    ld a,c
    cp 'A'
    jp c,ncBankEntryDriveReady
    cp 'H'+1
    jp nc,ncBankEntryDriveReady
    ld (ncGfxPath),a
    ld (ncEditorPath),a
    ld (ncAssociatedEditor),a
    ld (ncConfigPath),a
    ld (ncHelpPath),a
ncBankEntryDriveReady:
    ld a,(ncBankChildActive)
    ld (ncBankResumedChild),a
    xor a
    ld (ncBankChildActive),a
    call mx_memInfo
    ld (ncBankNumber),a
    ld a,b
    ld (ncBankRamPages),a
    ld a,(ncBankRegistered)
    or a
    jp nz,ncBankRetained
    ld de,NC_BANK_KEY
    ld a,RESIDENT_RETAIN
    MXOS_CALL MXOS_DOS_RESIDENT
    ret c
    ld a,1
    ld (ncBankRegistered),a
ncBankRetained:
    ld de,NC_BANK_KEY
    ld a,RESIDENT_SHELL_SET
    MXOS_CALL MXOS_DOS_RESIDENT
    ret c
    xor a
    MXOS_CALL MXOS_DOS_SET_SHELL
    ret c
    ld a,(ncBankInitOnly)
    or a
    jp z,ncBankEnterUI
    xor a
    ret
ncBankEnterUI:
    xor a
    call ncBankEncoding ; A legacy game may have left the font in KOI7.
    ld hl,NC_MAIN_PACKET
    ld de,NC_MAIN_END-NC_MAIN_PACKET
    ld b,0
    ld a,MEM_OP_MAINREGISTER
    MXOS_CALL MXOS_DOS_MEMORY
    ret c
    ld hl,(ncGfxHandle)
    ld a,h
    or l
    jp z,ncBankLoadGraphics
    call ncGfxBind
    jp nc,ncBankGraphicsReady
ncBankLoadGraphics:
    ; Зарегистрированный буфер учитывается при выборе базы DLL.
    ld hl,ncGfxPath
    ld a,MODULE_TARGET_MAIN
    MXOS_CALL MXOS_DOS_MODULE_LOAD
    jp c,ncBankGraphicsFailed
    ld (ncGfxHandle),hl
    call ncGfxBind
    jp c,ncBankGraphicsFailed
ncBankGraphicsReady:
    ; The scope pins GFX. External programs and unload run only after RET.
    xor a
    ld (ncBankExitAction),a
    ld (ncBankScopeError),a
    ; A warm restart can discard the scope and its timer lease while retaining NC.
    ld (ncInputState.timed),a
    MXOS_KEY_BEGIN ncInputState
    ld hl,(ncGfxHandle)
    ld de,ncBankController
    MXOS_CALL MXOS_DOS_MODULE_SCOPE
    push af
    MXOS_KEY_END ncInputState
    call ncBankFreePacket
    pop af
    ret c
    ld a,(ncBankScopeError)
    or a
    jp nz,ncBankScopeFailed
    ld a,(ncBankExitAction)
    cp 1
    jp z,ncBankExec
    cp 2
    jp z,ncBankFinishExit
    cp 3
    jp z,ncBankEdit
    xor a
    ret
ncBankScopeFailed:
    scf
    ret
ncBankExec:
    ; MXOS_EXEC leaves its register packet on this stack on return.
    ld hl,0
    add hl,sp
    ld (ncBankExecSP),hl
    ld hl,vars.cmdLine
    MXOS_EXEC ncBankExecDone
ncBankExecDone:
    ld hl,(ncBankExecSP)
    ld sp,hl
    xor a
    ret
; Synchronous child. Both UIs release the drawing scope before handing
; over MAIN. RET resumes this same NC, independently of the selected shell.
;----------------------------------------------------------------------------
; MXOS
;
; Запустить банковый редактор после выхода из области действия UI_GFX.
;
; вход:
;   vars.pathArg = путь ASCIIZ; ncBankNumber = страница NC.
;
; выход:
;   После синхронного вызова и попытки выгрузки EDITCORE устанавливаются ncEditorReturn
;   = 1 и ncEditorError = код ошибки или 0. Управление передаётся ncBankEnterUI.
;----------------------------------------------------------------------------

ncBankEdit:
    call ncBankReleaseGraphics
    ld hl,ncEditorPath
    ld (ncEditorErrorContext),hl
    ld a,MODULE_TARGET_BANK
    MXOS_CALL MXOS_DOS_MODULE_LOAD
    jp c,ncBankEditResult
    ld (ncEditorHandle),hl
    ld hl,ncEditorCallLabel
    ld (ncEditorErrorContext),hl
    ld hl,vars.pathArg
    ld (ncEditorRegs+2),hl
    ld a,1
    ld (ncEditorRegs+7),a
    ld a,(ncBankNumber)
    ld (ncEditorRegs+4),a
    ld a,(ncEditorPath)
    ld (ncEditorRegs+5),a
    ld hl,(ncEditorHandle)
    ld de,ncEditorRegs
    MXOS_CALL MXOS_DOS_MODULE_CALL
    jp c,ncBankEditUnload
    ld hl,(ncEditorRegs+6)
    push hl
    pop af
ncBankEditUnload:
    push af
    ld hl,(ncEditorHandle)
    xor a
    MXOS_CALL MXOS_DOS_MODULE_UNLOAD
    pop af
ncBankEditResult:
    jp c,ncBankEditStatus
    xor a
ncBankEditStatus:
    ld (ncEditorError),a
    ld a,1
    ld (ncEditorReturn),a
    jp ncBankEnterUI
ncBankFinishExit:
    call ncBankClose
    ld a,RESIDENT_RELEASE
    MXOS_CALL MXOS_DOS_RESIDENT
    ld a,1
    MXOS_CALL MXOS_DOS_SET_SHELL
    xor a
    ret
ncBankController:
    ld hl,0
    add hl,sp
    ld (ncBankReturnSP),hl
    ld hl,ncBankQueue
    ld bc,NC_GFX_QUEUE_BYTES
    ld de,NC_GFX_NORMAL_RECORD
    call commandQueueInit
    jp c,ncQAbort
    ld hl,(ncGfxHandle)
    ld a,MX_QUEUE_DLL
    call commandQueueSelect
    jp c,ncQFatal
    call ncGfxReadResult
    jp c,ncQFatal
    ld a,(ncGfxResult+13)
    ld (ncBankFontHeader+5),a
    ld a,(ncGfxResult+14)
    ld (ncBankFontHeader+6),a
    ld a,(ncGfxResult+15)
    ld (ncBankFontHeader+7),a
    ld a,(ncGfxResult+4)
    ld (ncBankCursorCfg),a
    ld hl,(ncGfxResult)
    ld (ncBankCursor),hl
    ld hl,0
    ld (ncBankInverse),hl
    ld a,(ncBankInitialized)
    or a
    jp nz,ncBankResume
    inc a
    ld (ncBankInitialized),a
    jp Start
ncBankResume:
    call ncLayoutInit
    call ncBankCheckEpoch
    ld hl,vars.cmdLine
    ld (hl),0
    ld (vars.cmdLineEnd),hl
ncBankResumeCommandReady:
    ld hl,ncDiskChanged
    ld a,CLI_OP_OBSERVER
    MXOS_CALL MXOS_DOS_CLI
    ld a,(ncEditorReturn)
    or a
    jp z,ncBankResumeClear
    xor a
    ld (ncEditorReturn),a
    COLOR COLOR_CMDLINE
    ld hl,090E6h
    ld b,48
    ld c,26
    xor a
    call uiFillRectBytes
    jp ncBankResumeToolbar
ncBankResumeClear:
    ld c,31
    call bios_printChar
ncBankResumeToolbar:
    call drawToolbar
    ld a,(ncEditorError)
    or a
    jp z,mainReload
    push af
    xor a
    ld (ncEditorError),a
    call mainPrepareReload
    call fullRedraw
    call mainResetReload
    ld hl,(ncEditorErrorContext)
    ld de,vars.input
ncBankEditErrorPath:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    or a
    jp nz,ncBankEditErrorPath
    pop af
    call dosErrorText
    jp copyFileShowError
ncBankGraphicsFailed:
    push af
    call ncBankReleaseGraphics
    call ncBankFreePacket
    pop af
    ret
ncBankFreePacket:
    ld hl,NC_MAIN_PACKET
    ld b,0
    ld a,MEM_OP_FREE
    MXOS_CALL MXOS_DOS_MEMORY
    ret
ncBankReturn:
    call ncBankFlush
    ld a,(ncBankChildActive)
    or a
    call z,ncBankSaveEpoch
    call commandQueueClose
    ld a,(cqError)
    or a
    jp nz,ncQAbort
    ld hl,(ncBankReturnSP)
    ld sp,hl
    xor a
    ret
ncBankReleaseGraphics:
    call ncBankSaveEpoch
    ld hl,(ncGfxHandle)
    ld a,h
    or l
    ret z
    xor a
    MXOS_CALL MXOS_DOS_MODULE_UNLOAD
    ld hl,0
    ld (ncGfxHandle),hl
    ld (ncGfxResultPtr),hl
    ld (ncGfxBase),hl
    xor a
    ret
;----------------------------------------------------------------------------
; MXOS
;
; Закрыть интерфейс резидентного NC в EXEC.
;
; вход:
;   Состояние хранится в ncBankInitialized и ncGfxHandle.
;
; выход:
;   Для инициализированного интерфейса снимается наблюдатель и сохраняется состояние
;   панелей. Предпринимается выгрузка UI_GFX. a = 0, cf = 0.
;----------------------------------------------------------------------------

ncBankClose:
    ld a,(ncBankInitialized)
    or a
    jp z,ncBankCloseResources
    call ncStopObserver
    call saveState
ncBankCloseResources:
    call ncBankReleaseGraphics
    xor a
    ret
ncBankSaveEpoch:
    ld hl,ncBankEpoch
    ld b,RESIDENT_EPOCH_ALL
    ld a,RESIDENT_DIRECTORY_EPOCH
    MXOS_CALL MXOS_DOS_RESIDENT
    ret
ncBankCheckEpoch:
    ld hl,ncBankNewEpoch
    ld b,RESIDENT_EPOCH_ALL
    ld a,RESIDENT_DIRECTORY_EPOCH
    MXOS_CALL MXOS_DOS_RESIDENT
    ld hl,ncBankEpoch
    ld de,ncBankNewEpoch
    ld bc,0800h
ncBankEpochDrive:
    push bc
    ld b,4
    xor a
    ld (ncBankEpochDiff),a
ncBankEpochByte:
    ld a,(de)
    cp (hl)
    jp z,ncBankEpochSameByte
    push af
    ld a,1
    ld (ncBankEpochDiff),a
    pop af
ncBankEpochSameByte:
    ld (hl),a
    inc hl
    inc de
    dec b
    jp nz,ncBankEpochByte
    pop bc
    ld a,(ncBankEpochDiff)
    or a
    jp z,ncBankEpochNextDrive
    push bc
    push de
    push hl
    ld a,c
    ld c,1
    ld de,0
    call ncDiskChanged
    pop hl
    pop de
    pop bc
ncBankEpochNextDrive:
    inc c
    dec b
    jp nz,ncBankEpochDrive
    ret
;----------------------------------------------------------------------------
; MXOS
;
; Настроить приём команд UI_GFX и подготовить отступ глифа.
;
; вход:
;   ncGfxHandle = дескриптор DLL в MAIN.
;
; выход:
;   Возвращается AF от GFX_BIND при его отказе или от GFX_GLYPH_PADDING после успешной
;   настройки.
;----------------------------------------------------------------------------

ncGfxBind:
    ld hl,NC_MAIN_PACKET
    ld (ncGfxRegisters),hl
    ld hl,NC_GFX_QUEUE_BYTES
    ld (ncGfxRegisters+4),hl
    ld a,GFX_BIND
    call ncGfxCall
    ret c
    ld hl,(ncGfxRegisters)
    ld (ncGfxResultPtr),hl
    ld hl,(ncGfxRegisters+2)
    ld (ncGfxBase),hl
    ld hl,'A'
    ld (ncGfxRegisters),hl
    ld hl,8
    ld (ncGfxRegisters+4),hl
    ld a,GFX_GLYPH_PADDING
;----------------------------------------------------------------------------
; MXOS
;
; Непосредственно вызвать диспетчер UI_GFX из EXEC.
;
; вход:
;   a = операция GFX_*; аргументы заданы в ncGfxRegisters, дескриптор DLL в ncGfxHandle.
;
; выход:
;   При отказе маршрутизации возвращается AF DOS. После выполненного вызова AF
;   извлекается из пакета результата.
;----------------------------------------------------------------------------

ncGfxCall:
    ld (ncGfxRegisters+7),a
    ld hl,(ncGfxHandle)
    ld de,ncGfxRegisters
    MXOS_CALL MXOS_DOS_MODULE_CALL
    ret c
    ld hl,(ncGfxRegisters+6)
    push hl
    pop af
    ret
; HL=MAIN source, DE=local destination, exactly eight bytes.
ncGfxReadBytes8:
    ld (ncQTransfer+1),hl
    ex de,hl
    ld (ncQTransfer+4),hl
    ld hl,8
    jp ncGfxReadTransfer
;----------------------------------------------------------------------------
; MXOS
;
; Скопировать результат непосредственного вызова UI_GFX из MAIN в EXEC.
;
; вход:
;   ncBankNumber = страница NC. ncGfxResultPtr указывает на GFX_RESULT_BYTES байт.
;
; выход:
;   При cf = 0 данные скопированы в ncGfxResult; при cf = 1 в a код ошибки mx_memcpyFar.
;----------------------------------------------------------------------------

ncGfxReadResult:
    ld hl,(ncGfxResultPtr)
    ld (ncQTransfer+1),hl
    ld hl,ncGfxResult
    ld (ncQTransfer+4),hl
    ld hl,GFX_RESULT_BYTES
ncGfxReadTransfer:
    ld (ncQTransfer+6),hl
    ld a,(ncBankNumber)
    ld (ncQTransfer+3),a
    xor a
    ld (ncQTransfer),a
    ld hl,ncQTransfer
    jp mx_memcpyFar
;----------------------------------------------------------------------------
; MXOS
;
; Вывести очередь NC в MAIN перед синхронной операцией.
;
; вход:
;   Активна область действия графического контроллера.
;
; выход:
;   При успехе регистры и флаги сохранены. При cqError область действия
;   завершается с кодом в ncBankScopeError.
;----------------------------------------------------------------------------

ncBankFlush:
    call mx_queueFlush
    push af
    ld a,(cqError)
    or a
    jp nz,ncQFatal
    pop af
    ret
ncQFatal:
    call commandQueueClose
    ld a,(cqError)
    or a
    jp nz,ncQAbort
    ld a,MEM_ERROR_RANGE
ncQAbort:
    ld (ncBankScopeError),a
    ld hl,(ncBankReturnSP)
    ld sp,hl
    ret

ncQueueCommand:
    push af
    push bc
    push de
    push hl
    ld (ncQRegisters),hl
    ex de,hl
    ld (ncQRegisters+2),hl
    ld h,b
    ld l,c
    ld (ncQRegisters+4),hl
    push af
    pop hl
    ld (ncQRegisters+6),hl
    ld hl,0
    ld (ncQPayloadSize),hl
    ld a,(ncQOperation)
    cp GFX_TEXT
    jp z,ncQTextSize
    cp GFX_DRAW
    jp z,ncQDrawSize
    cp GFX_COPY_COLORS
    jp z,ncQTextSize
    cp GFX_TOOLBAR
    jp z,ncQTextSize
    cp GFX_STORE
    jp z,ncQTextSize
    cp GFX_TILE_COLUMN
    jp z,ncQTileSize
    cp GFX_BITMAP_RESTORE_COLORS
    ld hl,12
    jp z,ncQPayloadReady
    jp ncQSizeReady
ncQTileSize:
    ld a,(ncQRegisters+4)
    ld l,a
    ld h,0
    jp ncQPayloadReady
ncQTextSize:
    ld hl,(ncQRegisters+4)
    jp ncQPayloadReady
ncQDrawSize:
    ld hl,(ncQRegisters)
    ld de,0
ncQDrawObject:
    ld a,(hl)
    inc hl
    inc de
    or a
    jp z,ncQDrawEnd
    and 15
    cp 1
    ld b,4
    jp nz,ncQDrawObjectBytes
    inc b
ncQDrawObjectBytes:
    inc hl
    inc de
    dec b
    jp nz,ncQDrawObjectBytes
    jp ncQDrawObject
ncQDrawEnd:
    ex de,hl
ncQPayloadReady:
    ld (ncQPayloadSize),hl
ncQSizeReady:
    ld hl,(ncQPayloadSize)
    ld de,GFX_RECORD_HEADER
    add hl,de
    ld a,h
    or a
    jp nz,ncQReserve
    ld a,l
    cp NC_GFX_NORMAL_RECORD+1
    jp c,ncQFastWrite
ncQReserve:
    ld b,h
    ld c,l
    call commandQueueReserve
    jp c,ncQFatal
    ld a,(cqError)
    or a
    jp nz,ncQFatal
    ex de,hl
    jp ncQWrite
ncQFastWrite:
    ld hl,(cqNext)
    ex de,hl
ncQWrite:
    ld a,(ncQOperation)
    ld (de),a
    inc de
    ld a,(ncCurrentColor)
    ld (de),a
    inc de
    ld hl,ncBankCursor
    ; Cursor, inverse and the four register words have fixed lengths.
    DUP 4
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    EDUP
    ld hl,ncQRegisters
    DUP 8
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    EDUP
    ld hl,(ncQPayloadSize)
    ld a,l
    ld (de),a
    inc de
    ld a,h
    ld (de),a
    inc de
    ld b,h
    ld c,l
    ld hl,(ncQRegisters)
    ld a,(ncQOperation)
    cp GFX_TILE_COLUMN
    jp nz,ncQPayload
    ld hl,(ncQRegisters+2)
ncQPayload:
    ld a,b
    or c
    jp z,ncQDone
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec bc
    jp ncQPayload
ncQDone:
    ex de,hl
    call commandQueueCommit
    ld a,(cqError)
    or a
    jp nz,ncQFatal
    pop hl
    pop de
    pop bc
    pop af
    ret
ncBankPrintRaw:
    push af
    push bc
    push de
    ld (ncQTextStart),hl
    ld bc,0
ncQTextCount:
    ld a,(hl)
    or a
    jp z,ncQTextCounted
    inc hl
    inc bc
    jp ncQTextCount
ncQTextCounted:
    push hl
    ld hl,(ncQTextStart)
    call ncBankPrintLim
    pop hl
    pop de
    pop bc
    pop af
    ret
ncBankPrintLim:
    push af
    ld a,GFX_TEXT
    ld (ncQOperation),a
    call ncQueueCommand
    pop af
    push af
    push bc
    push de
    ld a,(ncBankFontHeader+5)
    rrca
    ld d,a
ncQAdvance:
    ld a,b
    or c
    jp z,ncQAdvanceDone
    inc hl
    call ncBankAdvance
ncQAdvanceNext:
    dec bc
    jp ncQAdvance
ncQAdvanceDone:
    pop de
    pop bc
    pop af
    ret
ncBankAdvance:
    ld a,(ncBankCursor+1)
    add a,d
    cp 192
    jp c,ncQAdvanceX
    call ncBankDown
    xor a
ncQAdvanceX:
    ld (ncBankCursor+1),a
    ret
ncBankPrintString:
    push af
    push bc
ncQControlString:
    ld a,(hl)
    or a
    jp z,ncQControlDone
    ld c,a
    call ncBankPrintChar
    inc hl
    jp ncQControlString
ncQControlDone:
    pop bc
    pop af
    ret
ncBankPrintChar:
    push af
    ld a,GFX_CHAR
    ld (ncQOperation),a
    pop af
    call ncQueueCommand
    push af
    push de
    ld a,c
    cp 32
    jp nc,ncQCharAdvance
    cp 8
    jp z,ncQCharBack
    cp 10
    jp z,ncQCharNewline
    cp 13
    jp z,ncQCharReturn
    cp 12
    jp z,ncQCharHome
    cp 24
    jp z,ncQCharAdvance
    cp 25
    jp z,ncQCharUp
    cp 26
    jp z,ncQCharDown
    cp 31
    jp nz,ncQCharDone
ncQCharHome:
    ld a,8
    ld (ncBankCursor),a
ncQCharReturn:
    xor a
    ld (ncBankCursor+1),a
    jp ncQCharDone
ncQCharNewline:
    call ncBankDown
    jp ncQCharReturn
ncQCharUp:
    call ncBankUp
    jp ncQCharDone
ncQCharDown:
    call ncBankDown
    jp ncQCharDone
ncQCharBack:
    ld a,(ncBankFontHeader+5)
    rrca
    ld d,a
    ld a,(ncBankCursor+1)
    sub d
    jp nc,ncQCharBackStore
    ld a,192
    sub d
    ld (ncBankCursor+1),a
    call ncBankUp
    jp ncQCharDone
ncQCharBackStore:
    ld (ncBankCursor+1),a
    jp ncQCharDone
ncQCharAdvance:
    ld a,(ncBankFontHeader+5)
    rrca
    ld d,a
    call ncBankAdvance
ncQCharDone:
    pop de
    pop af
    ret
ncBankDown:
    ld a,(ncBankCursor)
    add a,10
    ret c ; BIOS scrolls and retains the old baseline at the bottom.
    ld (ncBankCursor),a
    ret
ncBankUp:
    ld a,(ncBankCursor)
    sub 10
    ret c
    cp 8
    ret c
    ld (ncBankCursor),a
    ret
ncBankSetCursor:
    ld (ncBankCursor),hl
    ret
;----------------------------------------------------------------------------
; MXOS
;
; Дождаться события клавиатуры в EXEC, обслуживая курсор, Shift и статус.
;
; вход:
;   Локальная клавиатура, графическая очередь и координаты курсора инициализированы.
;   Регистровых параметров нет.
;
; выход:
;   A = клавиша, CF = признак Ctrl, BC, DE и HL сохранены. Остальные флаги
;   не определены. Метка платформы обновляется только при основном вводе NC.
;----------------------------------------------------------------------------
ncBankGetch:
    push bc
    push de
    push hl
    call ncBankFlush
    ld hl,ncInputState
    call io_bankKeyTicks
    ld c,a
    ld b,2                 ; Show after a short idle interval.
    ld e,0                 ; Cursor is currently hidden.
ncBankInputPoll:
    call ncBankReadShift
    ld hl,ncShiftState
    cp (hl)
    call nz,ncBankInputShift
    ld hl,ncInputState
    ld d,(hl)              ; Previous raw key, preserved by io_bankKeyPoll.
    call io_bankKeyPoll
    jp nz,ncBankInputKey   ; F1 is zero in A, but NZ denotes an event.
    call ncStatusIdle
    call io_bankKeyTicks
    sub c
    cp b
    jp c,ncBankInputPoll
    ld a,(ncBankCursorCfg)
    or a
    jp m,ncBankInputIdle
    and 0Fh
    jp z,ncBankInputIdle
    call ncBankInputCursor
    inc e
ncBankInputIdle:
    ld hl,ncInputState
    call io_bankKeyTicks
    ld c,a
    ld b,KEY_CURSOR_HALF
    jp ncBankInputPoll
ncBankInputKey:
    push af
    call ncBankPanelRepeat
    ld a,e
    rrca
    call c,ncBankInputCursor
    pop af
    pop hl
    pop de
    pop bc
    ret
; CF=Ctrl, A=decoded key, D=previous raw key. One unit is 32.768 ms with the interval clock.
; A panel step includes a MAIN graphics batch. Keep the extra held-key
; wait short enough that it does not dominate a step without scrolling.
NC_PANEL_REPEAT_FIRST = 8
NC_PANEL_REPEAT_NEXT  = 1
ncBankPanelRepeat:
    ret c
    cp 008h               ; Left
    jp z,ncBankPanelArrow
    sub 018h              ; Right, Up, Down
    cp 3
    ret nc
ncBankPanelArrow:
    ld a,(ncDialogActive)
    ld hl,ncDamageMask
    or (hl)               ; Large overlays such as help use the redraw path.
    ret nz
    ld a,(ncInputState.lastKey)
    cp d
    ld a,NC_PANEL_REPEAT_FIRST
    jp nz,ncBankSetRepeat
    ld a,NC_PANEL_REPEAT_NEXT
ncBankSetRepeat:
    ld (ncInputState.repeatCount),a
    ret
ncBankInputShift:
    call ncBankDrawToolbarState
    jp ncBankFlush
; Keep the toolbar's existing encoding: 0 pressed, 2 released.
ncBankReadShift:
    call io_bankKeyShift
    xor IO_KEY_MOD_SHIFT
    add a,a
    ret
ncBankInputCursor:
    push bc
    push de
    ld hl,(ncBankCursor)
    ld (ncGfxRegisters),hl
    ld a,(ncCurrentColor)
    ld (ncGfxRegisters+4),a
    ld a,(ncBankCursorCfg)
    ld (ncGfxRegisters+5),a
    ld a,GFX_CURSOR
    ld (ncGfxRegisters+7),a
    ld hl,(ncGfxHandle)
    ld de,ncGfxRegisters
    call mx_routeCall
    jp c,ncQFatal
    pop de
    pop bc
    ret
ncBankEncoding:
    MXOS_CALL MXOS_BIOS_SET_FONT_ENCODING
    ret
ncBankGetMemTop:
    call mx_getMemTop
    ret
ncBankSyncCursor:
    push af
    push bc
    push de
    push hl
    call mx_getCursorPos
    ld (ncBankCursor),hl
    pop hl
    pop de
    pop bc
    pop af
    ret
ncBankDiskInfo:
    MXOS_CALL MXOS_DOS_RAW_IO
    ret
ncBankCompare EQU cqCompare
ncBankSubtract:
    ld a,l
    sub e
    ld l,a
    ld a,h
    sbc a,d
    ld h,a
    ret
    MODULE ncBankStrings
    INCLUDE "../common/strcmp.inc"
    ENDMODULE
    MODULE ncBankCopy
    INCLUDE "../common/memcpy_bc_hl.inc"
    INCLUDE "../common/move8080.inc"
    ENDMODULE
ncBankPrintHexWord:
    push af
    push bc
    push hl
    ld a,h
    call ncQHexByte
    ld a,l
    call ncQHexByte
    pop hl
    pop bc
    pop af
    ret
ncQHexByte:
    push af
    rrca
    rrca
    rrca
    rrca
    call ncQHexDigit
    pop af
ncQHexDigit:
    and 15
    add a,'0'
    cp '9'+1
    jp c,ncQHexPrint
    add a,7
ncQHexPrint:
    ld c,a
    jp ncBankPrintChar
ncMemoryInit:
    ld a,FILE_CACHE_CAPACITY
    ld (ncCatalogCapacity),a
    ld hl,FILE_CACHE_A
    ld (ncCatalogBases),hl
    ld hl,FILE_CACHE_B
    ld (ncCatalogBases+2),hl
    xor a
    ret
ncMemoryError:
    jp ncBankReturn
ncCatalogCapacity:      DB FILE_CACHE_CAPACITY
ncMemoryTop:            DW 0
ncMemoryEnd:            DW NC_END
ncBankNumber:           DB 0
ncBankInitialized:      DB 0
ncBankRegistered:       DB 0
ncBankInitOnly:         DB 0
ncBankChildActive:      DB 0
ncBankResumedChild:     DB 0
ncBankEpoch:            DS 32,0
ncBankNewEpoch:         DS 32,0
ncBankEpochDiff:        DB 0
ncBankReturnSP:         DW 0
ncBankExecSP:           DW 0
ncGfxHandle:            DW 0
ncGfxResultPtr:         DW 0
ncGfxBase:              DW 0
ncGfxRegisters:         DS MX_REGS,0
ncGfxResult:            DS GFX_RESULT_BYTES,0
ncEditorHandle:         DW 0
ncEditorErrorContext:   DW ncEditorPath
ncEditorCallLabel:      DB "EDIT",0
ncEditorRegs:           DS MX_REGS,0
ncEditorReturn:         DB 0
ncEditorError:          DB 0
ncEditorPath:           DB 'C:',05Ch,'DOS',05Ch,'EDITCORE.DLL',0
ncGfxPath:              DB 'C:',05Ch,'DOS',05Ch,'UI_GFX.DLL',0
ncBankExitAction:       DB 0
ncBankScopeError:       DB 0
ncQPayloadSize:         DW 0
ncQTextStart:           DW 0
ncQOperation:           DB 0
ncQRegisters:           DS MX_REGS,0
ncQTransfer:            DS MEM_COPY_REQUEST,0
ncInputState:           IO_KEY_STATE

    INCLUDE "../include/command-queue.inc"
