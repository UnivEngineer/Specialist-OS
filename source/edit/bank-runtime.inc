; EXEC document engine. SDK geometry stays local; bitmap primitives run in MAIN.
; A=0: launcher in MAIN. A=1: graphical caller, C=source FAR bank.
; DE addresses a MAX_PATH argument buffer in that bank in both cases.
;----------------------------------------------------------------------------
; MXOS
;
; Запустить EDITCORE.DLL и выполнить редактор в области действия UI_GFX.
;
; вход:
;   a = 0 при запуске из DOS, a = 1 при вызове из NC; c = страница источника, de = буфер
;   аргументов размером MAX_PATH. b = буква накопителя с EDITCORE и его оверлеями.
;
; выход:
;   При успехе a = 0, cf = 0; при отказе a = код, cf = 1. Аргументы копируются в EXEC,
;   графика выполняется в MAIN.
;----------------------------------------------------------------------------

editorBankEntry:
    ld (ebQuiet),a
    ld a,c
    ld (ebArgBank),a
    ex de,hl
    ld (ebArguments),hl
    call editorOverlayDrive
    ld bc,RESIDENT_PRIORITY_EDIT*256
    ld a,RESIDENT_POLICY
    MXOS_CALL_REGS MXOS_DOS_RESIDENT
    ret c
    ld c,1
    ld de,v_documentDirty
    ld hl,ebDirtyReason
    ld a,RESIDENT_VETO
    MXOS_CALL_REGS MXOS_DOS_RESIDENT
    ret c
    call mx_memInfo
    ld (ebNumber),a
    ld hl,(ebArguments)
    ld de,inputBuf
    ld bc,MAX_PATH
    ld a,(ebArgBank)
    call ebFromBank
    ret c
    call ebRegisterWorkspace
    ret c
    ld hl,ebGfxPath
    ld a,MODULE_TARGET_MAIN
    MXOS_CALL_REGS MXOS_DOS_MODULE_LOAD
    jp c,ebFreeWorkspace
    ld (ebGfxHandle),hl
    MXOS_KEY_BEGIN ebKeyState
    ld hl,(ebGfxHandle)
    ld de,ebController
    MXOS_CALL_REGS MXOS_DOS_MODULE_SCOPE
    push af
    MXOS_KEY_END ebKeyState
    ld hl,(ebGfxHandle)
    xor a
    MXOS_CALL_REGS MXOS_DOS_MODULE_UNLOAD
    pop af
ebFreeWorkspace:
    push af
    ld hl,EDIT_MAIN_PACKET
    ld b,0
    ld a,MEM_OP_FREE
    MXOS_CALL_REGS MXOS_DOS_MEMORY
    pop af
    ret c
    ld a,(ebStatus)
    or a
    ret z
    scf
    ret
ebController:
    ld hl,0
    add hl,sp
    ld (ebScopeSP),hl
    ld hl,EDIT_BANK_QUEUE
    ld bc,EDIT_QUEUE_BYTES
    ld de,16
    call commandQueueInit
    jp c,ebFailed
    ld hl,(ebGfxHandle)
    ld a,MX_QUEUE_DLL
    call commandQueueSelect
    jp c,ebFailed
    ld hl,EDIT_MAIN_PACKET
    ld (ebGfxRegisters),hl
    ld hl,EDIT_QUEUE_BYTES
    ld (ebGfxRegisters+4),hl
    ld a,GFX_BIND
    call ebCall
    jp c,ebFailed
    ld de,ebCursor
    ld bc,16
    call ebFromMain
    jp c,ebFailed
    ld hl,EDIT_MAIN_KEY_LOCKS
    ld b,0
    ld a,MEM_OP_GET8
    call mx_memory
    jp c,ebFailed
    ld a,e
    ld (ebKeyLocks),a
    ld a,COLOR_TEXT
    ld (ebColor),a
    ld hl,textOverlayFile
    call editorLoadOverlay
    jp c,ebFailed
    ld de,inputBuf
    jp openCommandLineFile
ebReturn:
    call ebFlush
    ld a,(ebKeyLocks)
    ld e,a
    ld hl,EDIT_MAIN_KEY_LOCKS
    ld b,0
    ld a,MEM_OP_PUT8
    call mx_memory
ebLeave:
    ld (ebStatus),a
    call commandQueueClose
    ld hl,(ebScopeSP)
    ld sp,hl
    ret
ebFailed:
    jp ebLeave
ebCall:
    ld (ebGfxRegisters+7),a
    ld hl,(ebGfxHandle)
    ld de,ebGfxRegisters
    call mx_routeCall
    ret
; HL=source MAIN, DE=destination EXEC, BC=bytes.
ebFromMain:
    xor a
ebFromBank:
    ld (ebTransfer),a
    ld (ebTransfer+1),hl
    ex de,hl
    ld (ebTransfer+4),hl
    ld h,b
    ld l,c
    ld (ebTransfer+6),hl
    ld a,(ebNumber)
    ld (ebTransfer+3),a
    ld hl,ebTransfer
    jp mx_memcpyFar
; HL=source EXEC, DE=destination MAIN, BC=bytes.
ebToMain:
    ld (ebTransfer+1),hl
    ex de,hl
    ld (ebTransfer+4),hl
    ld h,b
    ld l,c
    ld (ebTransfer+6),hl
    ld a,(ebNumber)
    ld (ebTransfer),a
    xor a
    ld (ebTransfer+3),a
    ld hl,ebTransfer
    jp mx_memcpyFar
;----------------------------------------------------------------------------
; MXOS
;
; Выполнить накопленные операции рисования в MAIN.
;
; вход:
;   Активна очередь банкового редактора.
;
; выход:
;   При успехе регистры и флаги сохранены. При ошибке ebFailed завершает область
;   действия контроллера с кодом из cqError.
;----------------------------------------------------------------------------

ebFlush:
    push af
    push bc
    push de
    push hl
    call commandQueueFlush
    ld a,(cqError)
    or a
    jp nz,ebFailed
    pop hl
    pop de
    pop bc
    pop af
    ret
;----------------------------------------------------------------------------
; MXOS
;
; Добавить команду графики в очередь банкового редактора.
;
; вход:
;   ebQOperation = операция GFX_*, регистры содержат её аргументы.
;
; выход:
;   Очередь дополнена снимком аргументов, курсора, цвета и данных. Регистры и флаги
;   сохранены. Ошибка резервирования передаётся ebFailed.
;----------------------------------------------------------------------------

ebQueueCommand:
    push af
    push bc
    push de
    push hl
    ld hl,0
    add hl,sp
    ld de,ebQRegisters
    ld b,8
ebQCapture:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,ebQCapture
    ld hl,0
    ld (ebQPayloadSize),hl
    ld a,(ebQOperation)
    cp GFX_SCROLLBAR
    ld hl,UI_SCROLLBAR
    jp z,ebQPayloadReady
    cp GFX_TEXT
    jp z,ebQTextSize
    IF EDIT_HEX_PSEUDO
    cp GFX_TEXT_PSEUDO
    jp z,ebQTextSize
    ENDIF
    cp GFX_TEXT_STORE
    jp z,ebQRowSize
    cp GFX_DRAW
    jp z,ebQDrawSize
    cp GFX_TOOLBAR
    jp z,ebQToolbarSize
    jp ebQSizeReady
ebQToolbarSize:
    ld hl,(ebQRegisters)
    ld de,0
ebQToolbarField:
    ld a,(hl)
    inc hl
    inc de
    or a
    jp z,ebQDrawEnd
ebQToolbarText:
    ld a,(hl)
    inc hl
    inc de
    or a
    jp nz,ebQToolbarText
    jp ebQToolbarField
ebQRowSize:
    ld a,(ebQRegisters+7)
    ld l,a
    ld h,0
    jp ebQPayloadReady
ebQTextSize:
    ld hl,(ebQRegisters+4)
    jp ebQPayloadReady
ebQDrawSize:
    ld hl,(ebQRegisters)
    ld de,0
ebQDrawObject:
    ld a,(hl)
    inc hl
    inc de
    or a
    jp z,ebQDrawEnd
    and 15
    cp 1
    ld b,4
    jp nz,ebQDrawObjectBytes
    inc b
ebQDrawObjectBytes:
    inc hl
    inc de
    dec b
    jp nz,ebQDrawObjectBytes
    jp ebQDrawObject
ebQDrawEnd:
    ex de,hl
ebQPayloadReady:
    ld (ebQPayloadSize),hl
ebQSizeReady:
    ld hl,(ebQPayloadSize)
    ld a,h
    or l
    jp nz,ebQReservePayload
    ld hl,(cqNext)
    jp ebQWrite
ebQReservePayload:
    ld de,16
    add hl,de
    ld b,h
    ld c,l
    call commandQueueReserve
    jp c,ebFailed
ebQWrite:
    ex de,hl
    ld a,(ebQOperation)
    ld (de),a
    inc de
    ld a,(ebColor)
    ld (de),a
    inc de
    ld hl,ebCursor
    ld b,4
ebQState:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,ebQState
    ld hl,ebQRegisters
    ld b,8
ebQRegs:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,ebQRegs
    ld hl,(ebQPayloadSize)
    ld a,l
    ld (de),a
    inc de
    ld a,h
    ld (de),a
    inc de
    ld b,h
    ld c,l
    ld hl,(ebQRegisters)
    ld a,(ebQOperation)
    cp GFX_TEXT_STORE
    jp nz,ebQPayload
    ld hl,(ebQRegisters+2)
ebQPayload:
    ld a,b
    or c
    jp z,ebQDone
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec bc
    jp ebQPayload
ebQDone:
    ex de,hl
    call commandQueueCommit
    pop hl
    pop de
    pop bc
    pop af
    ret
ebPrintRaw:
    push af
    push bc
    push de
    ld (ebQTextStart),hl
    ld bc,0
ebQTextCount:
    ld a,(hl)
    or a
    jp z,ebQTextCounted
    inc hl
    inc bc
    jp ebQTextCount
ebQTextCounted:
    push hl
    ld hl,(ebQTextStart)
    call ebPrintLim
    pop hl
    pop de
    pop bc
    pop af
    ret
ebPrintLim:
    push af
    ld a,GFX_TEXT
    ld (ebQOperation),a
    pop af
    call ebQueueCommand
    push af
    push bc
    push de
    ld a,(ebFont+5)
    rrca
    ld d,a
ebQAdvance:
    ld a,b
    or c
    jp z,ebQAdvanceDone
    inc hl
    call ebAdvance
ebQAdvanceNext:
    dec bc
    jp ebQAdvance
ebQAdvanceDone:
    pop de
    pop bc
    pop af
    ret
ebAdvance:
    ld a,(ebCursor+1)
    add a,d
    cp 192
    jp c,ebQAdvanceX
    call ebDown
    xor a
ebQAdvanceX:
    ld (ebCursor+1),a
    ret
ebPrintString:
    push af
    push bc
ebQControlString:
    ld a,(hl)
    or a
    jp z,ebQControlDone
    ld c,a
    call ebPrintChar
    inc hl
    jp ebQControlString
ebQControlDone:
    pop bc
    pop af
    ret
ebPrintChar:
    push af
    ld a,GFX_CHAR
    ld (ebQOperation),a
    pop af
    call ebQueueCommand
    push af
    push de
    ld a,c
    cp 32
    jp nc,ebQCharAdvance
    cp 8
    jp z,ebQCharBack
    cp 10
    jp z,ebQCharNewline
    cp 13
    jp z,ebQCharReturn
    cp 12
    jp z,ebQCharHome
    cp 24
    jp z,ebQCharAdvance
    cp 25
    jp z,ebQCharUp
    cp 26
    jp z,ebQCharDown
    cp 31
    jp nz,ebQCharDone
ebQCharHome:
    ld a,8
    ld (ebCursor),a
ebQCharReturn:
    xor a
    ld (ebCursor+1),a
    jp ebQCharDone
ebQCharNewline:
    call ebDown
    jp ebQCharReturn
ebQCharUp:
    call ebUp
    jp ebQCharDone
ebQCharDown:
    call ebDown
    jp ebQCharDone
ebQCharBack:
    ld a,(ebFont+5)
    rrca
    ld d,a
    ld a,(ebCursor+1)
    sub d
    jp nc,ebQCharBackStore
    ld a,192
    sub d
    ld (ebCursor+1),a
    call ebUp
    jp ebQCharDone
ebQCharBackStore:
    ld (ebCursor+1),a
    jp ebQCharDone
ebQCharAdvance:
    ld a,(ebFont+5)
    rrca
    ld d,a
    call ebAdvance
ebQCharDone:
    pop de
    pop af
    ret
ebDown:
    ld a,(ebCursor)
    add a,10
    ret c ; BIOS scrolls and retains the old baseline at the bottom.
    ld (ebCursor),a
    ret
ebUp:
    ld a,(ebCursor)
    sub 10
    ret c
    cp 8
    ret c
    ld (ebCursor),a
    ret
ebSetCursor:
    ld (ebCursor),hl
    ret

    MACRO EB_PRIMITIVE NAME,OP
NAME:
    push af
    ld a,OP
    ld (ebQOperation),a
    pop af
    jp ebQueueCommand
    ENDM
    EB_PRIMITIVE uiDraw,GFX_DRAW
    EB_PRIMITIVE uiFillRectPixels,GFX_FILL
    EB_PRIMITIVE uiFrameRectPixels,GFX_FRAME
    EB_PRIMITIVE uiInverseRectPixels,GFX_INVERSE
    EB_PRIMITIVE uiFillRectBytes,GFX_BYTES
    EB_PRIMITIVE ebRecolor,GFX_RECOLOR
    EB_PRIMITIVE uiCopyTextRect,GFX_COPY
    IF EDIT_HEX_PSEUDO
    EB_PRIMITIVE uiCopyRect,GFX_COPY_RECT
    ENDIF
uiToolbar:
    push de
    ld d,b
    ld e,c
    ld a,GFX_TOOLBAR
    ld (ebQOperation),a
    call ebQueueCommand
    pop de
    ret
    EB_PRIMITIVE uiDrawScrollbar,GFX_SCROLLBAR
; SDK hook: HL=generated absolute G_MODAL_WINDOW. Preserve every register.
; Serialize only its byte rectangle; capture/replay executes in MAIN.
ebSnapshotModal:
    push af
    ld a,(editorInterfaceReady)
    or a
    jp nz,ebSnapshotReady
    pop af
    ret
ebSnapshotReady:
    push bc
    push de
    push hl
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld a,(hl)
    add a,6
    ld c,a
    inc hl
    ld a,(hl)
    add a,2
    ld b,a
    ex de,hl
    ld (ebSnapshotScreen),hl
    push bc
    pop hl
    ld (ebSnapshotSize),hl
    ld hl,(ebSnapshotScreen)
    ld a,1
    ld (ebSnapshotActive),a
    ld de,EDIT_MAIN_PIXELS
    ld a,GFX_BITMAP_SAVE
    ld (ebQOperation),a
    call ebQueueCommand
    pop hl
    pop de
    pop bc
    pop af
    ret
ebDiscardModal:
    push af
    xor a
    ld (ebSnapshotActive),a
    pop af
    ret
ebRestoreModal:
    push af
    push bc
    push de
    push hl
    call ebRestoreSnapshot
    pop hl
    pop de
    pop bc
    pop af
    ret
ebRestoreSnapshot:
    ld a,(ebSnapshotActive)
    or a
    ret z
    call ebDiscardModal
    ld de,EDIT_MAIN_PIXELS
    ld hl,(ebSnapshotSize)
    ld b,h
    ld c,l
    ld hl,(ebSnapshotScreen)
    ; Resume document drawing with the same palette used to restore the bitmap.
    ld a,COLOR_TEXT
    ld (ebColor),a
    ld a,GFX_BITMAP_RESTORE
    ld (ebQOperation),a
    call ebQueueCommand
    ; Make the restored document visible before a following disk operation.
    jp ebFlush
ebSnapshotActive:   DB 0
ebSnapshotScreen:   DW 0
ebSnapshotSize:     DW 0
;----------------------------------------------------------------------------
; MXOS
;
; Дождаться клавиши после вывода графической очереди в MAIN.
;
; вход:
;   Настроено ebKeyState. ebHook задаёт обработчик смены регистров клавиатуры и
;   разрешает фоновую подготовку строк.
;
; выход:
;   a = клавиша, cf = состояние Ctrl/ПС при событии. bc, de и hl сохранены; видимый
;   курсор скрыт.
;----------------------------------------------------------------------------

ebGetch:
    push bc
    push de
    push hl
    call ebFlush
ebWaitReady:
    ld a,(ebKeyLocks)
    ld b,a
    xor a
    ld (ebCaretVisible),a
    call ebToggleCaret
    ld hl,ebKeyState
    call io_bankKeyTicks
    ld d,a
ebPollKey:
    ld hl,ebKeyState
    call io_bankKeyPoll    ; A = key, CF = Ctrl/PS on events
    jp nz,ebInputReady
    ld a,(ebKeyLocks)
    xor b
    and 1
    jp nz,ebInputChanged
    ld hl,(ebHook)
    ld a,h
    or l
    jp z,ebInputIdle
    call io_bankKeyShift
    ld hl,editorShiftState
    cp (hl)
    call nz,ebInputUpdate
    jp ebInputIdle
ebInputChanged:
    call ebInputUpdate
ebInputIdle:
    ld hl,ebKeyState
    call io_bankKeyTicks
    sub d
    cp KEY_CURSOR_HALF
    jp c,ebIdleSlice
    add a,d
    ld d,a
    call ebToggleCaret
    jp ebPollKey
ebIdleSlice:
    cp 3                      ; Leave the initial 98 ms to the keyboard.
    jp c,ebPollKey
    cp KEY_CURSOR_HALF-1      ; Reserve the last tick for the next blink.
    jp nc,ebPollKey
    ld hl,(ebHook)
    ld a,h
    or l                       ; No document prefetch in modal waits.
    jp z,ebPollKey
    push bc
    push de
    ld a,(hexActive)
    or a
    jp z,ebIdleText
    call hexPrefetchRow
    jp ebIdleDone
ebIdleText:
    call ebPrefetch
ebIdleDone:
    pop de
    pop bc
    jp ebPollKey
ebInputReady:
    push af
    ld a,(ebCaretVisible)
    or a
    call nz,ebToggleCaret
    pop af
    pop hl
    pop de
    pop bc
    ret

ebInputUpdate:
    ld a,(ebKeyLocks)
    ld b,a
    ld hl,(ebHook)
    ld a,h
    or l
    ret z
    call ebRunInputHook
    jp ebFlush
ebRunInputHook:
    jp (hl)

ebToggleCaret:
    ld a,(ebCursorCfg)
    and 08Fh                ; disabled, or a caret of zero height
    ret z
    ret m
    push bc
    push de
    ld hl,(ebCursor)
    ld (ebGfxRegisters),hl
    ld a,(ebColor)
    ld (ebGfxRegisters+4),a
    ld a,(ebCursorCfg)
    ld (ebGfxRegisters+5),a
    ld a,(ebCaretVisible)
    xor 1
    ld (ebCaretVisible),a
    ld a,GFX_CURSOR
    call ebCall
    jp c,ebFailed
    pop de
    pop bc
    ret
;----------------------------------------------------------------------------
; MXOS
;
; Заменить обработчик изменений состояния клавиатуры в EXEC.
;
; вход:
;   hl = новый адрес или 0 для отключения.
;
; выход:
;   hl = прежний адрес; af, bc и de сохранены.
;----------------------------------------------------------------------------

ebSetHook:
    push de
    ex de,hl
    ld hl,(ebHook)
    ex de,hl
    ld (ebHook),hl
    ex de,hl
    pop de
    ret
ebEncoding:
    ld (ebKeyState.koi7),a
    MXOS_CALL_REGS MXOS_BIOS_SET_FONT_ENCODING
    ret
ebMemTop:
    ld hl,EDITOR_MEMORY_END-1
    ret
    MODULE ebCopy
    INCLUDE "../common/memcpy_bc_hl.inc"
    INCLUDE "../common/move8080.inc"
    ENDMODULE
    INCLUDE "bank-cache.inc"
    INCLUDE "../../sdk/asm/lib/command-queue.inc"
ebNumber:           DB 0
ebQuiet:            DB 0
ebDirtyReason:      DB 'EDIT.COM',0,'Document has unsaved changes',0
ebArgBank:          DB 0
ebArguments:        DW 0
ebGfxHandle:        DW 0
ebGfxRegisters:     DS MX_REGS,0
ebGfxPath:          DB 'C:',05Ch,'DOS',05Ch,'UI_GFX.DLL',0
ebScopeSP:          DW 0
ebStatus:           DB 0
ebQPayloadSize:     DW 0
ebQTextStart:       DW 0
ebQOperation:       DB 0
ebQRegisters:       DS MX_REGS,0
ebTransfer:         DS MEM_COPY_REQUEST,0
ebHook:             DW 0
ebKeyState:         IO_KEY_STATE
ebKeyLocks = ebKeyState.locks
ebCaretVisible: DB 0
