; All long operations use FILEOPS; no screen clear or recursive UI code.
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
    ld a,13
    ld (ncProgressKey),a
    call ncProgressClock.pitTimerInit
    jp c,ncFileopsBegin
    call ncProgressClock.pitTimerRead
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
    ld a,FILEOPS_ANSWER
    call ncFileopsCall
    jp c,ncFileopsAbort
    call ncProgressClock.pitTimerRead
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
ncFileopsCall:
    ld hl,vars.fileJob
    MXOS_CALL MXOS_DOS_FILEOPS
    ret
; Only newly pressed keys activate buttons; held Enter from the input is ignored.
ncProgressInput:
    call bios_keyScan
    ld b,a
    ld a,(ncProgressKey)
    cp b
    ld a,b
    ld (ncProgressKey),a
    jp z,ncProgressStep
    or a
    jp z,ncProgressStep
    cp 01Bh
    jp z,ncProgressCancelKey
    cp 8
    jp z,ncProgressChoose
    cp 018h
    jp z,ncProgressChoose
    cp 9
    jp z,ncProgressChoose
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
ncProgressChoose:
    ld a,(vars.fileJob.phase)
    cp FILEOPS_PHASE_PREPARE
    jp nz,ncProgressStep
    COLOR COLOR_DIALOG
    xor a
    call ncProgressSelectedFrame
    ld a,(ncProgressChoice)
    xor 1
    ld (ncProgressChoice),a
    ld a,0FFh
    call ncProgressSelectedFrame
    jp ncProgressStep
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
    call ncProgressClock.pitTimerRead
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
    call ncProgressClock.pitTimerRead
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
    ld hl,ncProgressWindow
    call ncDialogOpen
    COLOR COLOR_DIALOG
    ld hl,ncProgressWindow
    call draw
    ld a,1
    ld (ncProgressVisible),a
ncProgressContent:
    COLOR COLOR_DIALOG
    ld hl,68
    ld bc,248
    ld d,93
    ld e,71
    xor a
    call uiFillRectPixels
    ld hl,ncProgressDividers
    call draw
    ld a,0FFh
    ld (ncProgressPercent),a
    ld (ncProgressLastPath),a
    ld hl,ncProgressLastNumber
    ld b,6
ncProgressResetNumber:
    ld (hl),a
    inc hl
    dec b
    jp nz,ncProgressResetNumber
    xor a
    ld (ncProgressBarWidth),a
    ld a,(vars.fileJob.phase)
    ld (ncProgressPhase),a
    call ncProgressButtons
    ld a,(vars.fileJob.phase)
    cp FILEOPS_PHASE_PREPARE
    jp z,ncProgressPreparing
    ld hl,02467h                ; X=72 px
    call bios_setCursorPos
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
    ld e,103
    call ncPrintCentered
    jp ncProgressValues
ncProgressPreparing:
    ld a,(vars.fileJob.mode)
    cp FILEOPS_MOVE
    ld hl,ncProgressPrepareTitle
    jp nz,ncProgressPrepareSelected
    ld hl,ncProgressPrepareMoveTitle
ncProgressPrepareSelected:
    ld e,103
    call ncPrintCentered
    jp ncProgressValues
 ; Static title/buttons survive heartbeats. Text is opaque and padded,
; only changed values are printed. The bar only grows, in 8-pixel pieces.
ncProgressValues:
    COLOR COLOR_DIALOG
    ld a,(vars.fileJob.phase)
    cp FILEOPS_PHASE_PREPARE
    jp z,ncProgressScanValues
    ld c,0
    ld hl,vars.fileJob.bytes
    ld de,ncProgressLastNumber
    ld b,4
    call ncProgressCompare
    ld a,c
    or a
    jp z,ncProgressValuesBar
    call ncProgressTextBegin
    call ncProgressCounts
    ld hl,02478h
    call bios_setCursorPos
    ld hl,240
    call ncProgressFieldWidth
    ld hl,ncProgressText
    call ncPrintRightField
ncProgressValuesBar:
    ld a,(vars.fileJob.known)
    or a
    call nz,ncProgressBar
    jp ncProgressPathLine
ncProgressScanValues:
    ld c,0
    ld hl,vars.fileJob.total
    ld de,ncProgressLastNumber
    ld b,4
    call ncProgressCompare
    ld hl,vars.fileJob.files
    ld de,ncProgressLastNumber+4
    ld b,2
    call ncProgressCompare
    ld a,c
    or a
    jp z,ncProgressPathLine
    call ncProgressTextBegin
    ld hl,(vars.fileJob.files)
    ex de,hl
    call ncProgressAppendWord
    ld hl,ncProgressFilesText
    call ncProgressAppend
    ld hl,vars.fileJob.total
    call ncProgressBytes
    ld hl,02478h
    call bios_setCursorPos
    ld hl,240
    call ncProgressFieldWidth
    ld hl,ncProgressText
    call ncPrintField
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
    ld hl,0248Fh
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
; Compare and remember B bytes; C accumulates differences across fields.
ncProgressCompare:
    ld a,(de)
    xor (hl)
    or c
    ld c,a
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,ncProgressCompare
    ret
ncProgressButtons:
    ld a,(ncProgressVisible)
    or a
    ret z
    ld a,(vars.fileJob.phase)
    cp FILEOPS_PHASE_PREPARE
    jp nz,ncProgressOneButton
    xor a
    ld (ncProgressCancelWidget.selected),a
    ld (ncProgressAbortButton.selected),a
    ld hl,ncProgressCancelWidget
    call ncDrawButton
    ld hl,ncProgressAbortButton
    call ncDrawButton
    ld a,1
    jp ncProgressSelectedFrame
ncProgressSelectedFrame:
    and 1
    push af
    ld a,(ncProgressChoice)
    or a
    ld hl,ncProgressCancelWidget
    jp z,ncProgressFocusReady
    ld hl,ncProgressAbortButton
ncProgressFocusReady:
    pop af
    jp uiSelectButton
ncProgressOneButton:
    ld hl,ncProgressOnlyAbortButton
    jp ncDrawButton
ncProgressCancelWidget: UI_BUTTON ncProgressCancel,36,56,149,14,6,0
ncProgressAbortButton: UI_BUTTON ncProgressAbort,100,56,149,14,6,0
ncProgressOnlyAbortButton: UI_BUTTON ncProgressAbortEsc,68,56,149,14,6,1
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
    ld bc,194
    ld d,123
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
    ld hl,08C83h                ; Right edge at X=312, baseline 131.
    call bios_setCursorPos
    ld hl,ncProgressText
    ld b,4
    call ncPrintRightField
    ld a,(ncProgressPercent)
    ld l,a
    ld h,0
    ld de,24
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
    ld d,124
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
    ld a,'/'
    call ncProgressAppendChar
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
ncProgressSubtract:
    ld a,l
    sub e
    ld l,a
    ld a,h
    sbc a,d
    ld h,a
    ret
ncProgressWindow:
    G_MODAL_WINDOW 80h,64,88,256,80
    DB 0
ncProgressDividers:
    G_LINE 80h,68,108,248
    G_LINE 80h,68,147,248
    DB 0
ncProgressTitle: DB 'Copying files...',0
ncProgressMoveTitle: DB 'Moving files...',0
ncProgressPrepareTitle: DB 'Preparing to copy files...',0
ncProgressPrepareMoveTitle: DB 'Preparing to move files...',0
ncProgressDeleteTitle: DB 'Deleting files...',0
ncProgressCancel: DB 'Cancel [Esc]',0
ncProgressAbort: DB 'Abort [A]',0
ncProgressAbortEsc: DB 'Abort [Esc]',0
ncProgressFilesText: DB ' files / ',0
ncProgressB: DB ' B',0
ncProgressKiBText: DB ' KiB',0
ncProgressMiB: DB ' MiB',0
ncProgressVisible: DB 0
ncProgressTimer: DB 0
ncProgressPhase: DB 0FFh
ncProgressChoice: DB 0
ncProgressKey: DB 0
ncProgressPercent: DB 0
ncProgressStart: DW 0
ncProgressOut: DW 0
ncProgressBarWidth: DB 0
ncProgressLastNumber: DS 6,0FFh
ncProgressLastPath: DS 61,0FFh
ncProgressUnits: DW 0
ncProgressDenom: DS 4,0
ncProgressNumer: DS 4,0
ncProgressNumber: DS 4,0
ncProgressRemainder: DS 4,0
ncProgressText EQU vars.displayPath
ncProgressPath EQU vars.pathArg ; Reuse the existing modal path buffer.
    MODULE ncProgressClock
    INCLUDE "../include/timing-pit.inc"
    ENDMODULE

