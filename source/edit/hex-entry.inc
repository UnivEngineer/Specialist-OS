; Bridge between the text and HEX views. Both keep logical file positions.
hexActive:          DB 0
hexRestoreLF:       DB 0
hexMode6:           DB 0        ; CS, KOI7-R, KOI8-R, All
hexFiltered:        DB 1        ; active glyph filter
hexTop:             DW 0        ; first physical screen row in the dump
hexCaret:           DW 0        ; file offset, including EOF
hexChangedBreak:    DB 0
editorPendingHex:   DB 0
hexUnsupported:     DB 'Invalid/unsupported executable',0
    INCLUDE "hex-container.inc"

; A nonmoving command such as Save or Help keeps the half-CRLF position.
hexCheckTextCaret:
    ld a,(hexRestoreLF)
    or a
    ret z
    call hexAbsoluteCaret
    ex de,hl
    ld hl,(v_currentLine)
    push de
    ex de,hl
    ld hl,(v_column)
    add hl,de
    inc hl
    pop de
    call compareHLDE
    ret z
    xor a
    ld (hexRestoreLF),a
    ret

;----------------------------------------------------------------------------
; MXOS
;
; Пометить документ изменённым и сбросить сохранённую позицию внутри CRLF.
;
; вход:
;   Регистровых параметров нет.
;
; выход:
;   v_documentDirty = 1, hexRestoreLF = 0. a = 1, cf = 0, zf = 0; bc, de и hl сохранены.
;----------------------------------------------------------------------------

editorSetDirty:
    xor a
    ld (hexRestoreLF),a
    inc a
    ld (v_documentDirty),a
    ret

;----------------------------------------------------------------------------
; MXOS
;
; Получить логический адрес позиции редактирования для выделения блока.
;
; вход:
;   Текущий режим задан hexActive; в текстовом режиме учитывается hexRestoreLF.
;
; выход:
;   hl = логический адрес, включая EOF. При сохранённой позиции внутри CRLF возвращается
;   адрес LF.
;----------------------------------------------------------------------------

editorMarkPointer:
    ld a,(hexActive)
    or a
    jp nz,hexAbsoluteCaret
    call getLineBufferCursor
    ld a,(hexRestoreLF)
    or a
    ret z
    inc hl
    ret
hexAbsoluteCaret:
    ld hl,(hexCaret)
    ex de,hl
    ld hl,(v_textBeginLo)
    add hl,de
    ret
hexClamp:
    push hl
    ld hl,(hexSize)
    ex de,hl
    pop hl
    call compareHLDE
    ret c
    ex de,hl
    ret
hexSetGoal:
    ld hl,(hexCaret)
    call hexGrid
    ld a,(hexByteMask)
    and l
    ld (hexGoal),a
    ret
editorRedrawMark:
    ld a,(hexActive)
    or a
    jp nz,hexMarkMoved
    jp redrawPreservingCursor

hexToolbar:
    ld a,(editorShiftState)
    or a
    ld hl,hexBlockKeys
    jp nz,editorToolbarReady
    ld de,editorToolbarMark
    ld a,(editorMarkActive)
    or a
    jp z,hexMarkFrozen
    ld de,editorToolbarEnd
    jp hexSetLabel
hexMarkFrozen:
    ld hl,(v_selectionBegin)
    ld a,h
    or l
    jp z,hexSetLabel
    ld de,editorToolbarUnmark
hexSetLabel:
    ld hl,hexMarkLabel
    call editorToolbarCopyLabel
    ld hl,hexKeys
    jp editorToolbarReady

hexSkipHalf:
    ld a,(hexAlign)
    ld c,a
    ld a,(hexAcross)
    and c
    ret

hexGridFile:
    ld a,e
    or a
    ret z
    push de
    ld a,(hexAlign)
    ld e,a
    ld d,0
    call bios_sub_hl_de
    jp nc,hexGridFileReady
    ld hl,0
hexGridFileReady:
    pop de
    add hl,de
    ret

hexToggle:
    call testShiftKey
    ret z
;----------------------------------------------------------------------------
; MXOS
;
; Войти в HEX и по завершении восстановить текстовое представление.
;
; вход:
;   Подготовлено состояние документа и курсора. MAIN использует встроенный HEX; EXEC
;   загружает EDITHEX.DLL после вывода очереди.
;
; выход:
;   a = 0, cf = 0 после возврата к тексту. В EXEC при отказе входа показан диалог, cf =
;   1.
;----------------------------------------------------------------------------

hexEnter:
    IF EDIT_BANKED
    call ebFlush
    ld hl,hexOverlayFile
    call editorSwitchOverlay
    jp c,hexEnterError
    call ebInvalidateRows
    ld hl,EDIT_HEX_QUEUE_BYTES
    call hexQueueCapacity
    ENDIF
    call editorMarkPointer
    push hl
    ld hl,(v_textBeginLo)
    ex de,hl
    pop hl
    call bios_sub_hl_de
    ld (hexCaret),hl
    xor a
    ld (hexChangedBreak),a
    ld a,1
    ld (hexActive),a
    ld (editorInterfaceReady),a
    call hexView
    IF EDIT_BANKED
hexLeaveTry:
    call ebFlush
    ld hl,textOverlayFile
    call editorSwitchOverlay
    jp nc,hexLeaveReady
    call hexOpenError
    ld hl,0FFFFh
    ld (hexCachedLine),hl
    call hexWait
    jp hexLeaveTry
hexLeaveReady:
    ENDIF
    xor a
    ld (hexActive),a
    ld (hexRestoreLF),a
    IF EDIT_BANKED
    call ebInvalidateRows
    ENDIF
    call hexAbsoluteCaret
    call editorInsideCRLF
    jp nz,hexFindTextLine
    dec hl
    ld a,1
    ld (hexRestoreLF),a
hexFindTextLine:
    push hl
    ld hl,(v_textBeginLo)
    ex de,hl
    pop hl
    push hl
hexTextStart:
    call compareHLDE
    jp z,hexTextFound
    dec hl
    call gapTextRead
    cp 13
    jp nz,hexTextStart
    inc hl
hexTextFound:
    ld (v_currentLine),hl
    ex de,hl
    pop hl
    call bios_sub_hl_de
    ld (v_column),hl
    call hexRestoreTextGeometry
    IF EDIT_BANKED
    call ebFlush
    ld hl,EDIT_QUEUE_BYTES
    call hexQueueCapacity
    ENDIF
    call recountCurrentLine
    call editorRevealCurrentLine
    call editorResetGoal
    ld a,1                    ; reset the document's horizontal track area
    ld (editorHShown),a
    call clearTextViewport
    call editorToolbar
    call drawStatusBar
    call redrawPreservingCursor
    xor a
    ret

    IF EDIT_BANKED
hexEnterError:
    call hexOpenError
    xor a
    ld (editorPendingHex),a
    scf
    ret

; Flush before changing capacity. MAIN's receive allocation stays intact.
hexQueueCapacity:
    ld (cqAllocation),hl
    jp cqSetCapacity
; B = drive containing EDITCORE. Update both overlay paths, preserving BC/DE/HL.
editorOverlayDrive:
    ld a,b
    ld (hexCorePath),a
    ld (textCorePath),a
    ret

; Back up the overlapping work area so a failed switch leaves the active
; code, text row widths and cache tags intact. No modal bitmap is live here.
;----------------------------------------------------------------------------
; MXOS
;
; Заменить оверлей с резервным копированием перекрываемой области.
;
; вход:
;   hl = дескриптор из четырёх слов: путь ASCIIZ, назначение, размер, контрольная сумма.
;   Буфер EDIT_MAIN_PIXELS свободен.
;
; выход:
;   cf = 0 при успехе. При отказе cf = 1, a = код; исходная область сохранена. Ошибка её
;   восстановления завершает контроллер через ebFailed.
;----------------------------------------------------------------------------

editorSwitchOverlay:
    push hl
    ld hl,EDIT_HEX_OVERLAY
    ld de,EDIT_MAIN_PIXELS
    ld bc,EDIT_BANK_STATE-EDIT_HEX_OVERLAY
    call ebToMain
    pop hl
    ret c
    call editorLoadOverlay
    ret nc
    push af
    ld hl,EDIT_MAIN_PIXELS
    ld de,EDIT_HEX_OVERLAY
    ld bc,EDIT_BANK_STATE-EDIT_HEX_OVERLAY
    call ebFromMain
    jp c,ebFailed
    pop af
    ret
    ASSERT EDIT_BANK_STATE-EDIT_HEX_OVERLAY <= 32*148

;----------------------------------------------------------------------------
; MXOS
;
; Загрузить и проверить оверлей в текущей странице EXEC.
;
; вход:
;   hl = дескриптор из четырёх слов: путь ASCIIZ, назначение, размер, контрольная сумма.
;
; выход:
;   cf = 0 при успехе. cf = 1, a = код при ошибке файла, проверки или возврата на
;   прежний диск. Запись может начаться до проверки.
;----------------------------------------------------------------------------

editorLoadOverlay:
    ld (editorOverlayFile),hl
    ld e,0
    MXOS_CALL MXOS_DOS_FILE_GET_SET_DRIVE
    push af
    call hexReadOverlay
    pop bc
    push af
    ld a,b
    MXOS_CALL MXOS_DOS_FILE_SELECT_DRIVE
    jp c,hexRestoreError
    pop af
    ret
hexRestoreError:
    pop af
    ret c
    MXOS_CALL MXOS_DOS_GET_ERROR
    ret
hexReadOverlay:
    call flClear
    ld hl,(editorOverlayFile)
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld (flHandle+FH_BUFFER),hl
    ld a,FILE_MODE_READ
    ld (flHandle+FH_MODE),a
    ld a,FILE_OP_OPEN_PATH
    call flCall
    ret c
    ld hl,IMAGE_HEADER_SIZE
    ld (flHandle+FH_SEEK),hl
    ld a,FILE_OP_SEEK
    call flCall
    jp c,flClose
    ld hl,(editorOverlayFile)
    inc hl
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ex de,hl
    ld (flHandle+FH_BUFFER),hl
    ex de,hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ex de,hl
    ld (flHandle+FH_COUNT),hl
    ld (editorOverlayLength),hl
    ex de,hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld (editorOverlayChecksum),hl
    ld a,FILE_OP_READ
    call flCall
    call flClose
    ret c
    ld hl,(flHandle+FH_COUNT)
    ex de,hl
    ld hl,(editorOverlayLength)
    call compareHLDE
    jp nz,hexBadOverlay
    ld hl,(flHandle+FH_BUFFER)
    push hl
    add hl,de
    dec hl
    ex de,hl
    pop hl
    call hexLocal.calcCS
    ld d,b
    ld e,c
    ld hl,(editorOverlayChecksum)
    call compareHLDE
    ret z
hexBadOverlay:
    ld a,DOS_ERR_BAD_IMAGE
    scf
    ret
hexOpenError:
    ld de,hexLoadError
    call dosErrorText
    call editorErrorWithTitle
    jp editorCloseModal
hexLoadError:   DB 'Cannot load editor view',0
hexCorePath:    DB 'C:',05Ch,'DOS',05Ch,'EDITHEX.DLL',0
textCorePath:   DB 'C:',05Ch,'DOS',05Ch,'EDITTEXT.DLL',0
    IFNDEF EDIT_HEX_CS
EDIT_HEX_CS  = 0
EDIT_TEXT_CS = 0
    ENDIF
hexOverlayFile:         DW hexCorePath,EDIT_HEX_OVERLAY,hexOverlayBytes,EDIT_HEX_CS
textOverlayFile:        DW textCorePath,EDIT_TEXT_OVERLAY,textOverlayBytes,EDIT_TEXT_CS
editorOverlayFile:      DW 0
editorOverlayLength:    DW 0
editorOverlayChecksum:  DW 0
    MODULE hexLocal
    INCLUDE "../common/calcCS.inc"
    ENDMODULE
    ENDIF
