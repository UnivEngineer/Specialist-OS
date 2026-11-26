; All long operations use FILEOPS; no screen clear or recursive UI code.
;----------------------------------------------------------------------------
; MXOS
;
; Выполнить файловую операцию из NC через пошаговый интерфейс FILEOPS.
;
; вход:
;   a = CLI_OP_DELETE, CLI_OP_COPY или CLI_OP_MOVE; vars.copyRequest заполнен. Вход
;   ncFileopsMode принимает a = FILEOPS_* напрямую.
;
; выход:
;   При успехе или отмене a = 0, cf = 0. При ошибке a = код, cf = 1. Цикл обслуживает
;   диалоги и клавиши.
;----------------------------------------------------------------------------

ncFileops:
    cp CLI_OP_DELETE
    ld b,FILEOPS_DELETE
    jp z,ncFileopsSelected
    cp CLI_OP_COPY
    ld b,FILEOPS_COPY
    jp z,ncFileopsSelected
    ld b,FILEOPS_MOVE
ncFileopsSelected:
    ld a,b
ncFileopsMode:
    ld (vars.fileJob.mode),a
    ld hl,vars.copyRequest
    ld de,vars.copyRequest+FILE_COPY_REQUEST
    ld bc,vars.fileJob.request
    call bios_memcpy_bc_hl
    xor a
    ld (vars.fileJob.flags),a
    ld (ncProgressVisible),a
    ld (ncProgressTimer),a
    ld (ncProgressChoice),a
    ld hl,ncProgressPath
    ld (vars.fileJob.path),hl
    IF NC_BANKED
    ld a,(ncInputState.timed)
    or a
    jp nz,ncFileopsClockReady
    ELSE
    ld a,13
    ld (ncProgressKey),a
    ENDIF
    ld a,TIMER_CLOCK
    MXOS_CALL_REGS MXOS_DOS_TIMER
    jp c,ncFileopsBegin
    call ncFileopsClockReady
    push af
    ld a,TIMER_RELEASE
    MXOS_CALL_REGS MXOS_DOS_TIMER
    pop af
    ret
ncFileopsClockReady:
    call ncProgressReadClock
    ld (ncProgressStart),hl
    ld a,1
    ld (ncProgressTimer),a
ncFileopsBegin:
    call ncDialogClose
    ld a,FILEOPS_BEGIN
    call ncFileopsCall
    ret c
ncFileopsLoop:
    call ncProgressInput
    call ncFileopsCall
    jp c,ncFileopsAbort
    ld a,(vars.fileJob.state)
    cp FILEOPS_CONFIRM
    jp z,ncFileopsConfirm
    cp FILEOPS_DONE
    jp nc,ncFileopsDone
    call ncProgressUpdate
    jp ncFileopsLoop
ncFileopsConfirm:
    ld hl,vars.fileJob.request.destination
    call ncOverwriteBody
    ld (vars.fileJob.answer),a
    xor a
    ld (ncProgressVisible),a
    call ncProgressPaint
    ld a,FILEOPS_ANSWER
    call ncFileopsCall
    jp c,ncFileopsAbort
    ld a,(ncProgressTimer)
    or a
    jp z,ncFileopsInspect
    call ncProgressReadClock
    ld (ncProgressStart),hl
    jp ncFileopsInspect
ncFileopsInspect:
    ld a,(vars.fileJob.state)
    cp FILEOPS_DONE
    jp nc,ncFileopsDone
    jp ncFileopsLoop
ncFileopsDone:
    ld a,(vars.fileJob.error)
    push af
    ld a,FILEOPS_CLOSE
    call ncFileopsCall
    call ncDialogClose
    pop af
    cp DOS_ERR_CANCELLED
    jp z,ncFileopsCancelled
    or a
    ret z
    scf
    ret
ncFileopsCancelled:
    xor a
    ret
ncFileopsAbort:
    push af
    ld a,FILEOPS_CLOSE
    call ncFileopsCall
    call ncDialogClose
    pop af
    scf
    ret
;----------------------------------------------------------------------------
; MXOS
;
; Выполнить один запрос DOS FILEOPS для задания NC.
;
; вход:
;   a = операция FILEOPS_*; vars.fileJob содержит описание задания.
;
; выход:
;   DOS получает hl = vars.fileJob. После вызова задание и AF содержат результат
;   FILEOPS. EXEC предварительно выводит очередь графики.
;----------------------------------------------------------------------------

ncFileopsCall:
    IF NC_BANKED
    call ncBankFlush
    ENDIF
    ld hl,vars.fileJob
    MXOS_CALL_REGS MXOS_DOS_FILEOPS
    ret
; Only newly pressed keys activate buttons; held Enter from the input is ignored.
;----------------------------------------------------------------------------
; MXOS
;
; Выбрать следующий шаг задания по клавиатуре и фазе FILEOPS.
;
; вход:
;   vars.fileJob.phase = фаза. Учитывается фокус видимого окна подготовки. MAIN
;   сканирует клавиатуру BIOS, EXEC опрашивает io_bankKeyPoll.
;
; выход:
;   a = FILEOPS_STEP, FILEOPS_SKIP_PREPARE или FILEOPS_CANCEL. Повторы клавиши
;   пропускаются; флаги не являются результатом.
;----------------------------------------------------------------------------

ncProgressInput:
    IF NC_BANKED
    ld hl,ncInputState
    ld a,(hl)
    push af
    call io_bankKeyPoll
    pop bc                ; B = preceding raw key. POP BC preserves Z.
    jp z,ncProgressStep
    ld c,a
    ld a,(hl)
    cp b                  ; A repeat must not activate a button again.
    jp z,ncProgressStep
    ld a,c
    ELSE
    call bios_keyScan
    ld b,a
    ld a,(ncProgressKey)
    cp b
    ld a,b
    ld (ncProgressKey),a
    jp z,ncProgressStep
    ENDIF
    or a
    jp z,ncProgressStep
    cp 01Bh
    jp z,ncProgressCancelKey
    ld e,a
    ld a,(ncProgressVisible)
    or a
    jp z,ncProgressNonFocus
    ld a,(vars.fileJob.phase)
    cp FILEOPS_PHASE_PREPARE
    jp nz,ncProgressNonFocus
    ld a,e
    call uiModalFocusKey
    jp nc,ncProgressAction
    ld a,(uiModalFocusIndex)
    ld (ncProgressChoice),a
    jp ncProgressStep
ncProgressNonFocus:
    ld a,e
ncProgressAction:
    cp 13
    jp z,ncProgressEnter
    and 0DFh
    cp 'A'
    jp z,ncProgressAbortKey
    cp 'C'
    jp z,ncProgressCancelButton
ncProgressStep:
    ld a,FILEOPS_STEP
    ret
ncProgressCancelButton:
    ld a,(vars.fileJob.phase)
    cp FILEOPS_PHASE_PREPARE
    jp nz,ncProgressStep
    ld a,FILEOPS_SKIP_PREPARE
    ret
ncProgressEnter:
    ld a,(ncProgressChoice)
    or a
    jp nz,ncProgressAbortKey
ncProgressCancelKey:
    ld a,(vars.fileJob.phase)
    cp FILEOPS_PHASE_PREPARE
    ld a,FILEOPS_SKIP_PREPARE
    ret z
ncProgressAbortKey:
    ld a,FILEOPS_CANCEL
    ret
; Paint at most four times a second; phase changes are immediate once visible.
ncProgressUpdate:
    ld a,(ncProgressVisible)
    or a
    jp z,ncProgressThrottle
    ld a,(vars.fileJob.phase)
    ld hl,ncProgressPhase
    cp (hl)
    jp nz,ncProgressPaint
ncProgressThrottle:
    ld a,(ncProgressTimer)
    or a
    jp z,ncProgressPaint
    call ncProgressReadClock
    ex de,hl
    ld hl,(ncProgressStart)
    call ncProgressSubtract
    ld de,1954
    call ncProgressSubtract
    ret c
ncProgressPaint:
    call ncProgressPaintBody
    ld a,(ncProgressTimer)
    or a
    ret z
    call ncProgressReadClock
    ld (ncProgressStart),hl
    ret
ncProgressPaintBody:
    ld a,(ncProgressVisible)
    or a
    jp z,ncProgressOpen
    ld a,(vars.fileJob.phase)
    ld hl,ncProgressPhase
    cp (hl)
    jp z,ncProgressValues
    jp ncProgressContent
ncProgressOpen:
    call ncProgressInitFocus
    call ncProgressModal
    call ncModalOpen
    ld a,1
    ld (ncProgressVisible),a
    jp ncProgressFocusDone
ncProgressContent:
    COLOR COLOR_DIALOG
    call ncProgressInitFocus
    call ncProgressModal
    call uiModalClear
ncProgressFocusDone:
    ld a,0FFh
    ld (ncProgressPercent),a
    ld (ncProgressLastPath),a
    xor a
    ld (ncProgressBarWidth),a
    ld a,(vars.fileJob.phase)
    ld (ncProgressPhase),a
    ld a,(vars.fileJob.phase)
    cp FILEOPS_PHASE_PREPARE
    jp z,ncProgressPreparing
    ld a,(vars.fileJob.phase)
    cp FILEOPS_PHASE_DELETE
    ld hl,ncProgressDeleteTitle
    jp z,ncProgressTitleReady
    ld hl,ncProgressTitle
ncProgressTitleReady:
    ld a,(vars.fileJob.mode)
    cp FILEOPS_MOVE
    jp nz,ncProgressTitleSelected
    ld hl,ncProgressMoveTitle
ncProgressTitleSelected:
    call uiModalTitle
    jp ncProgressValues
ncProgressPreparing:
    ld a,(vars.fileJob.mode)
    cp FILEOPS_MOVE
    ld hl,ncProgressPrepareTitle
    jp nz,ncProgressPrepareSelected
    ld hl,ncProgressPrepareMoveTitle
ncProgressPrepareSelected:
    call uiModalTitle
    jp ncProgressValues
 ; Static title/buttons survive heartbeats. Text is opaque and padded,
; only changed values are printed. The bar only grows, in 8-pixel pieces.
ncProgressValues:
    COLOR COLOR_DIALOG
    call ncProgressTextBegin
    ld hl,ncProgressFilesLabel
    call ncProgressAppend
    ld hl,(vars.fileJob.files)
    ex de,hl
    call ncProgressAppendWord
    ld a,(vars.fileJob.phase)
    cp FILEOPS_PHASE_PREPARE
    jp z,ncProgressFilesReady
    ld a,(vars.fileJob.known)
    or a
    jp z,ncProgressFilesReady
    ld hl,ncProgressSlash
    call ncProgressAppend
    ld hl,(vars.fileJob.totalFiles)
    ex de,hl
    call ncProgressAppendWord
ncProgressFilesReady:
    ld a,7
    call uiModalBodyY
    ld h,36
    ld l,e
    call bios_setCursorPos
    ld hl,96
    call ncProgressFieldWidth
    ld hl,ncProgressText
    call ncPrintField
    call ncProgressTextBegin
    ld hl,ncProgressSizeLabel
    call ncProgressAppend
    ld a,(vars.fileJob.phase)
    cp FILEOPS_PHASE_PREPARE
    jp nz,ncProgressTransferSize
    ld hl,vars.fileJob.total
    call ncProgressBytes
    jp ncProgressSizeReady
ncProgressTransferSize:
    call ncProgressCounts
ncProgressSizeReady:
    ld a,7
    call uiModalBodyY
    ld h,84
    ld l,e
    call bios_setCursorPos
    ld hl,144
    call ncProgressFieldWidth
    ld hl,ncProgressText
    call ncPrintRightField
    ld a,(vars.fileJob.phase)
    cp FILEOPS_PHASE_PREPARE
    jp z,ncProgressPathLine
    ld a,(vars.fileJob.known)
    or a
    jp z,ncProgressActivity
    call ncProgressBar
    jp ncProgressPathLine
; Unknown totals (DELETE or skipped preparation): activity, without a fake %.
ncProgressActivity:
    ld hl,79
    ld bc,226
    ld a,10
    call uiModalBodyY
    ld d,e
    ld e,11
    ld a,0FFh
    call uiFrameRectPixels
    ld hl,80
    ld bc,224
    ld a,11
    call uiModalBodyY
    ld d,e
    ld e,9
    xor a
    call uiFillRectPixels
    ld a,(ncProgressActivityX)
    add a,8
    cp 192
    jp c,ncProgressActivityReady
    xor a
ncProgressActivityReady:
    ld (ncProgressActivityX),a
    ld l,a
    ld h,0
    ld de,80
    add hl,de
    ld bc,24
    ld a,11
    call uiModalBodyY
    ld d,e
    ld e,9
    ld a,0FFh
    call uiFillRectPixels
    jp ncProgressPathLine
ncProgressPathLine:
    ld hl,240
    call ncProgressFieldWidth
    ld hl,ncProgressPath
    call compactPath
    push hl
    ld de,ncProgressLastPath
    ld c,0
ncProgressPathCompare:
    ld a,(de)
    xor (hl)
    or c
    ld c,a
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    or a
    jp nz,ncProgressPathCompare
    pop hl
    ld a,c
    or a
    ret z
    push hl
    ld a,30
    call uiModalBodyY
    ld h,36
    ld l,e
    call bios_setCursorPos
    ld hl,240
    call ncProgressFieldWidth
    pop hl
    jp ncPrintField
ncProgressFieldWidth:
    ld a,(ncFontWidth)
    ld e,a
    ld d,0
    call ncMath.div_hl_de
    ld b,l
    ret
ncProgressModal:
    ld a,(vars.fileJob.phase)
    cp FILEOPS_PHASE_PREPARE
    ld hl,ncProgressPrepareModal
    ret z
    ld hl,ncProgressRunModal
    ret
ncProgressInitFocus:
    call ncProgressModal
    ld a,0
    jp nz,ncProgressFocusIndex
    ld a,(ncProgressChoice)
ncProgressFocusIndex:
    ld bc,UI_FOCUS_GRID*256
    jp uiModalFocusInit
ncProgressPrepareModal:
    UI_MODAL_BEGIN 36,1,2
    UI_MODAL_BUTTON ncProgressCancel,0,0,1
    UI_MODAL_BUTTON ncProgressAbort,0,1,0
    UI_MODAL_END
ncProgressRunModal:
    UI_MODAL_BEGIN 36,1,1
    UI_MODAL_BUTTON ncProgressAbortEsc,0,0,1
    UI_MODAL_END
; Normalize to 16 bits <=655, preserving the ratio to about 0.2 percent.
ncProgressBar:
    ld hl,vars.fileJob.total
    ld de,vars.fileJob.total+4
    ld bc,ncProgressDenom
    call bios_memcpy_bc_hl
    ld hl,vars.fileJob.bytes
    ld de,vars.fileJob.bytes+4
    ld bc,ncProgressNumer
    call bios_memcpy_bc_hl
ncProgressScale:
    ld hl,(ncProgressDenom+2)
    ld a,h
    or l
    jp nz,ncProgressScaleOne
    ld hl,(ncProgressDenom)
    ld de,656
    call bios_cmp_hl_de
    jp c,ncProgressRatio
ncProgressScaleOne:
    ld hl,ncProgressDenom+3
    call ncProgressShift
    ld hl,ncProgressNumer+3
    call ncProgressShift
    jp ncProgressScale
ncProgressRatio:
    ld a,h
    or l
    ret z
    ex de,hl
    push de
    ld hl,(ncProgressNumer)
    ld de,100
    call ncMath.mul_hl_de
    pop de
    call ncMath.div_hl_de
    ld a,h
    or a
    ld a,l
    jp nz,ncProgressClamp
    cp 101
    jp c,ncProgressPercentReady
ncProgressClamp:
    ld a,100
ncProgressPercentReady:
    ld hl,ncProgressPercent
    cp (hl)
    ret z
    ld c,a
    ld a,(hl)
    cp 0FFh
    push bc
    jp nz,ncProgressBarFrameReady
    ld hl,79
    ld bc,226
    ld a,10
    call uiModalBodyY
    ld d,e
    ld e,11
    ld a,0FFh
    call uiFrameRectPixels
ncProgressBarFrameReady:
    pop bc
    ld a,c
    ld (ncProgressPercent),a
    call ncProgressTextBegin
    ld a,(ncProgressPercent)
    ld e,a
    ld d,0
    call ncProgressAppendWord
    ld a,'%'
    call ncProgressAppendChar
    ld hl,(uiModalCurrent)
    ld l,(hl)
    ld h,140
    call bios_setCursorPos
    ld hl,ncProgressText
    ld b,4
    call ncPrintRightField
    ld a,(ncProgressPercent)
    ld l,a
    ld h,0
    ld de,28
    call ncMath.mul_hl_de
    ld de,100
    call ncMath.div_hl_de
    add hl,hl
    add hl,hl
    add hl,hl                  ; 24 steps of eight pixels, 192 total.
    ld a,(ncProgressBarWidth)
    ld e,a
    ld d,0
    ld a,l
    sub e
    ret z
    ret c
    ld c,a
    ld b,0
    ld a,l
    ld (ncProgressBarWidth),a
    ld hl,80
    add hl,de
    ld a,11
    call uiModalBodyY
    ld d,e
    ld e,9
    ld a,0FFh
    jp uiFillRectPixels
; HL=u32 value. Render B below 1 KiB, otherwise KiB/MiB with one decimal.
ncProgressBytes:
    ld de,ncProgressNumber
    ld b,4
ncProgressNumberCopy:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,ncProgressNumberCopy
    ld hl,(ncProgressNumber+2)
    ld a,h
    or l
    jp nz,ncProgressKiB
    ld hl,(ncProgressNumber)
    ld de,1024
    call bios_cmp_hl_de
    jp nc,ncProgressKiB
    ex de,hl
    call ncProgressAppendWord
    ld hl,ncProgressB
    jp ncProgressAppend
ncProgressKiB:
    ld hl,ncProgressNumber
    ld de,ncProgressNumber+4
    ld bc,ncProgressRemainder
    call bios_memcpy_bc_hl
    ld c,10
    call ncProgressNumberShift
    ld hl,ncProgressKiBText
    ld (ncProgressUnits),hl
    ld hl,(ncProgressNumber+2)
    ld a,h
    or l
    jp nz,ncProgressUseMiB
    ld hl,(ncProgressNumber)
    ld de,1024
    call bios_cmp_hl_de
    jp c,ncProgressUnitsReady
ncProgressUseMiB:
    ld hl,ncProgressNumber
    ld de,ncProgressNumber+4
    ld bc,ncProgressRemainder
    call bios_memcpy_bc_hl
    ld c,10
    call ncProgressNumberShift
    ld hl,ncProgressMiB
    ld (ncProgressUnits),hl
ncProgressUnitsReady:
    ld hl,(ncProgressNumber)
    ex de,hl
    call ncProgressAppendWord
    ld a,'.'
    call ncProgressAppendChar
    ld hl,(ncProgressRemainder)
    ld a,h
    and 3
    ld h,a
    ld de,10
    call ncMath.mul_hl_de
    ld de,1024
    call ncMath.div_hl_de
    ld a,l
    add a,'0'
    call ncProgressAppendChar
    ld hl,(ncProgressUnits)
    jp ncProgressAppend
ncProgressCounts:
    ld hl,vars.fileJob.bytes
    ld a,(vars.fileJob.known)
    or a
    jp z,ncProgressBytes
    ld hl,(vars.fileJob.total+2)
    ld a,h
    or a
    jp nz,ncProgressCountMiB
    ld a,l
    cp 16
    jp nc,ncProgressCountMiB
    or a
    jp nz,ncProgressCountKiB
    ld hl,(vars.fileJob.total)
    ld de,1024
    call bios_cmp_hl_de
    jp nc,ncProgressCountKiB
    xor a
    ld hl,ncProgressB
    jp ncProgressCountUnit
ncProgressCountKiB:
    ld a,10
    ld hl,ncProgressKiBText
    jp ncProgressCountUnit
ncProgressCountMiB:
    ld a,20
    ld hl,ncProgressMiB
ncProgressCountUnit:
    ld (ncProgressCountShift),a
    ld (ncProgressUnits),hl
    ld hl,vars.fileJob.bytes
    call ncProgressInteger
    ld hl,ncProgressSlash
    call ncProgressAppend
    ld hl,vars.fileJob.total
    call ncProgressInteger
    ld hl,(ncProgressUnits)
    jp ncProgressAppend
ncProgressInteger:
    push hl
    ld de,4
    add hl,de
    ex de,hl
    pop hl
    ld bc,ncProgressNumber
    call bios_memcpy_bc_hl
    ld a,(ncProgressCountShift)
    or a
    jp z,ncProgressIntegerReady
    ld c,a
    call ncProgressNumberShift
ncProgressIntegerReady:
    ld hl,(ncProgressNumber)
    ex de,hl
    jp ncProgressAppendWord
ncProgressCountShift: DB 0

;----------------------------------------------------------------------------
; MXOS
;
; Начать новую строку состояния файловой операции.
;
; вход:
;   ncProgressText доступен для записи. Регистровых параметров нет.
;
; выход:
;   hl и ncProgressOut указывают на начало пустой ASCIIZ-строки. af, bc и de сохранены.
;----------------------------------------------------------------------------

ncProgressTextBegin:
    ld hl,ncProgressText
    ld (ncProgressOut),hl
    ld (hl),0
    ret
ncProgressAppendWord:
    ld b,0
    call ncFormatDecimal
ncProgressAppend:
    push hl
    ld hl,(ncProgressOut)
    ex de,hl
    pop hl
ncProgressAppendLoop:
    ld a,(hl)
    ld (de),a
    or a
    jp z,ncProgressAppendDone
    inc hl
    inc de
    jp ncProgressAppendLoop
ncProgressAppendDone:
    ex de,hl
    ld (ncProgressOut),hl
    ret

;----------------------------------------------------------------------------
; MXOS
;
; Дописать символ в строку состояния файловой операции.
;
; вход:
;   a = символ; ncProgressOut указывает на терминатор, в буфере есть место ещё для
;   одного байта и нуля.
;
; выход:
;   На прежнее место нуля записан символ. hl и ncProgressOut указывают на новый нуль.
;   af, bc и de сохранены.
;----------------------------------------------------------------------------

ncProgressAppendChar:
    ld hl,(ncProgressOut)
    ld (hl),a
    inc hl
    ld (hl),0
    ld (ncProgressOut),hl
    ret
ncProgressNumberShift:
    push bc
    ld hl,ncProgressNumber+3
    call ncProgressShift
    pop bc
    dec c
    jp nz,ncProgressNumberShift
    ret
ncProgressShift:
    ld b,4
    or a
ncProgressShiftByte:
    ld a,(hl)
    rra
    ld (hl),a
    dec hl
    dec b
    jp nz,ncProgressShiftByte
    ret

;----------------------------------------------------------------------------
; MXOS
;
; Вычесть два 16-битных счётчика.
;
; вход:
;   hl = уменьшаемое, de = вычитаемое.
;
; выход:
;   hl = разность по модулю 65536, a = старший байт результата, cf = заём. bc и de
;   сохранены.
;----------------------------------------------------------------------------

ncProgressSubtract:
    ld a,l
    sub e
    ld l,a
    ld a,h
    sbc a,d
    ld h,a
    ret
ncProgressTitle:                DB 'Copying files...',0
ncProgressMoveTitle:            DB 'Moving files...',0
ncProgressPrepareTitle:         DB 'Preparing to copy files...',0
ncProgressPrepareMoveTitle:     DB 'Preparing to move files...',0
ncProgressDeleteTitle:          DB 'Deleting files...',0
ncProgressCancel:               DB 'Cancel [Esc]',0
ncProgressAbort:                DB 'Abort [A]',0
ncProgressAbortEsc:             DB 'Abort [Esc]',0
ncProgressFilesLabel:           DB 'Files: ',0
ncProgressSizeLabel:            DB 'Size: ',0
ncProgressSlash:                DB '/',0
ncProgressActivityX:            DB 0
ncProgressB:                    DB ' B',0
ncProgressKiBText:              DB ' KiB',0
ncProgressMiB:                  DB ' MiB',0
ncProgressVisible:              DB 0
ncProgressTimer:                DB 0
ncProgressPhase:                DB 0FFh
ncProgressChoice:               DB 0
    IF !NC_BANKED
ncProgressKey: DB 0
    ENDIF
ncProgressPercent:      DB 0
ncProgressStart:        DW 0
ncProgressOut:          DW 0
ncProgressBarWidth:     DB 0
ncProgressLastPath:     DS 61,0FFh
ncProgressUnits:        DW 0
ncProgressDenom:        DS 4,0
ncProgressNumer:        DS 4,0
ncProgressNumber:       DS 4,0
ncProgressRemainder:    DS 4,0
ncProgressText EQU vars.displayPath
ncProgressPath EQU vars.pathArg  ; Reuse the existing modal path buffer.
    IF NC_BANKED
ncProgressReadClock = io_bankClockRead
    ELSE
ncProgressReadClock = io_pitRead
    ENDIF

