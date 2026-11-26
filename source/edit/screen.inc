; Примитивы памяти, курсор, экран и строка состояния
; Порядок включения задан главным .asm; см. README.md.

; Z means Shift is released in both profiles.
testShiftKey:
    IF EDIT_BANKED
        JP io_bankKeyShift
    ELSE
        LD A,(IO_KEYB_B)
        CPL
        RRCA
        AND 1
        RET
    ENDIF
; Вход: BC = источник, HL = приёмник, DE = конец приёмника (исключая).
; Выход: HL = DE; DE сохранён, AF/BC разрушены.
; Диапазон выделения не пересекается с новым вставленным диапазоном.
copyUntilDE:
        ; Block insertion has already placed the gap just after the new
        ; destination. The nonintersecting selection lies wholly on one side.
        PUSH DE
        CALL subtractHLFromDE
        PUSH DE
        CALL gapPhysical
        PUSH HL
        LD H,B
        LD L,C
        CALL gapPhysical
        POP BC
        POP DE
        PUSH HL
        ADD HL,DE
        EX DE,HL
        POP HL
        CALL bios_memcpy_bc_hl
        POP DE
        LD H,D
        LD L,E
        RET
; Вход: HL, DE. Выход: флаги сравнения HL с DE; пары сохранены, A разрушен.
compareHLDE:  LD    A,H           ; [34]
        CP    D
        RET   NZ
        LD    A,L
        CP    E
        RET

;----------------------------------------------------------------------------
; MXOS
;
; Вычесть адрес или смещение HL из DE.
;
; вход:
;   de = уменьшаемое, hl = вычитаемое.
;
; выход:
;   de = разность по модулю 65536. a = старший байт результата. cf = 1 при заёме. hl и
;   bc сохранены.
;----------------------------------------------------------------------------

subtractHLFromDE:  LD    A,E           ; [9]
        SUB   L
        LD    E,A
        LD    A,D
        SBC   A,H
        LD    D,A
        RET

        ; Звуковой сигнал 4 раза через п/п печати символа 07h
Beep4:
        LD    BC,0407H      ; b = счетчик цикла, c = код символа

        ; Печать b раз символа с кодом c
TypeCharCtimesB:
        CALL  bios_printChar
        DEC   B
        JP    NZ,TypeCharCtimesB
        RET

; HL logical -> next logical line. A=separator width, or zero at EOF.
; Ordinary printable bytes use one physical scan, crossing the gap on demand.
nextLine:
        CALL gapPhysical
editorScanByte:
        LD A,(HL)
        CP 14
        INC HL
        JP NC,editorScanByte
        DEC HL
        OR A
        CALL Z,gapTextZero
        OR A
        JP Z,editorScanEnd
        CP 13
        JP Z,editorScanSeparator
        CP 10
        JP Z,editorScanSeparator
        INC HL
        JP editorScanByte
editorScanSeparator:
        CALL gapLogical
        JP editorSkipSeparator
editorScanEnd:
        CALL gapLogical
        XOR A
        RET

;----------------------------------------------------------------------------
; MXOS
;
; Запомнить координаты курсора для restoreCursor.
;
; вход:
;   В bios_vars.cursorY и следующем байте хранятся текущие Y и X. Операнд
;   восстановления доступен для записи.
;
; выход:
;   Координаты записаны в операнд smc_restoreCursor. Регистры и флаги сохранены.
;----------------------------------------------------------------------------

saveCursor:  PUSH  HL            ; [7]
        LD    HL,(bios_vars.cursorY)
        ; САМОМОДИФИКАЦИЯ: сохранение курсора в операнде LD HL,nn.
        LD    (smc_restoreCursor+1),HL
        POP   HL
        RET
restoreCursor:  PUSH  HL            ; [7]
smc_restoreCursor:   LD    HL,0000H
        LD    (bios_vars.cursorY),HL
        POP   HL
        RET
resumeAtTextPointer:
        CALL resetSessionStack
        CALL previousLine
        LD (v_currentLine),HL
        LD (v_topLine),HL
        CALL recountCurrentLine
        JP clearAndRedrawDocument
updateEditMode:
updateLineNumber:
        RET
nextCurrentLine:  LD    HL,(v_currentLine)   ; [5]
        CALL  nextLine
        OR A
        RET Z
        LD    (v_currentLine),HL
        LD    HL,(v_lineNumber)
        INC   HL
        LD    (v_lineNumber),HL
        XOR   A
        INC   A
        RET
previousCurrentLine:  LD    HL,(v_currentLine)   ; [3]
        CALL  previousLine
        RET   Z
        LD    (v_currentLine),HL
        LD    HL,(v_lineNumber)
        DEC   HL
        LD    (v_lineNumber),HL
        RET
recountCurrentLine:  LD    HL,(v_currentLine)   ; [3]
        EX    DE,HL
        LD    HL,(v_textBeginLo)
        LD    (v_currentLine),HL
        LD    HL,0001H    ; 1
        LD    (v_lineNumber),HL
recountLineLoop: LD    HL,(v_currentLine)
        CALL  compareHLDE
        RET   Z
        CALL  nextCurrentLine
        JP    recountLineLoop
drawStatusBar:
        PUSH HL
        PUSH DE
        PUSH BC
        LD HL,(bios_vars.cursorY)
        PUSH HL
        CALL editorLayoutStatus
        COLOR COLOR_STATUSBAR
        LD HL,09000h
        LD B,48
        LD C,10
        LD A,255
        CALL uiFillRectBytes
        CALL enableInverse
        LD C,0
        CALL editorStatusPosition
        LD HL,editorStatusLnText
        CALL bios_printString
        LD A,(editorStatusColPos)
        LD C,A
        CALL editorStatusPosition
        LD HL,editorStatusColText
        CALL bios_printString
        LD A,(editorStatusTabPos)
        LD C,A
        CALL editorStatusPosition
        LD HL,editorStatusTabText
        CALL bios_printString
        LD A,(editorStatusLangPos)
        SUB 7
        LD C,A
        CALL editorStatusPosition
        LD HL,editorEncodingLabel
        CALL bios_printString
        LD HL,0FFFFh
        LD (editorLastLine),HL
        LD (editorLastCol),HL
        LD A,255
        LD (editorLastColHi),A
        LD (editorLastTab),A
        LD (editorLastLanguage),A
        LD A,2
        LD (editorLastMode),A
        CALL disableInverse
        CALL updateStatusBar
        JP editorStatusRestore

; Leave room for the six-character encoding label before RUS/LAT.
editorLayoutStatus:
        LD A,(editorScreenColumns)
        RRCA
        RRCA
        SUB 2
        LD (editorStatusColPos),A
        LD B,A
        ADD A,A
        LD (editorStatusTabPos),A
        ADD A,B
        LD (editorStatusModePos),A
        LD A,(editorScreenColumns)
        SUB 3
        LD (editorStatusLangPos),A
        ; v_charStep is pixels / 2. Derive both widths in screen bytes.
        LD A,(v_charStep)
        LD B,A
        ADD A,A
        ADD A,B
        INC A
        RRA                    ; ceil(6 glyphs / 8 pixels)
        LD (editorStatusNumBytes),A
        LD A,B
        ADD A,A
        ADD A,B
        RRA
        OR A
        RRA                    ; floor(3 glyphs / 8 pixels)
        LD (editorStatusTabBytes),A
        RET

; Only changed fixed-width values are printed; caption pixels stay intact.
updateStatusBar:
        PUSH HL
        PUSH DE
        PUSH BC
        LD HL,(bios_vars.cursorY)
        PUSH HL
        LD HL,(editorLastLine)
        EX DE,HL
        LD HL,(v_lineNumber)
        CALL compareHLDE
        JP Z,editorStatusColumn
        LD (editorLastLine),HL
        EX DE,HL
        LD C,4
        CALL editorStatusNumber
editorStatusColumn:
        CALL editorMeasureCursor
        INC HL
        LD B,A
        LD A,H
        OR L
        JP NZ,editorStatusColumnCarryDone
        INC B
editorStatusColumnCarryDone:
        LD A,(editorLastColHi)
        CP B
        JP NZ,editorStatusPrintColumn
        PUSH HL
        LD HL,(editorLastCol)
        EX DE,HL
        POP HL
        CALL compareHLDE
        JP Z,editorStatusTab
editorStatusPrintColumn:
        LD (editorLastCol),HL
        LD A,B
        LD (editorLastColHi),A
        CALL editorFormatFour
        LD A,(editorStatusColPos)
        ADD A,4
        LD C,A
        CALL editorStatusDigits
editorStatusTab:
        LD A,(editorLastTab)
        LD B,A
        LD A,(v_tabWidth)
        CP B
        JP Z,editorStatusMode
        LD (editorLastTab),A
        ADD A,'0'
        LD (editorStatusTabValue),A
        LD A,(editorStatusTabPos)
        ADD A,4
        LD C,A
        LD A,(editorStatusTabBytes)
        LD D,A
        LD HL,editorStatusTabValue
        CALL editorStatusField
editorStatusMode:
        LD A,(editorLastMode)
        LD B,A
        LD A,(v_insertMode)
        CP B
        JP Z,editorStatusLanguageCheck
        LD (editorLastMode),A
        LD A,(editorStatusModePos)
        LD C,A
        CALL editorStatusPosition
        COLOR COLOR_STATUSBAR
        CALL enableInverse
        LD A,(v_insertMode)
        OR A
        LD HL,txtIns
        JP NZ,editorStatusPrintMode
        LD HL,txtOvr
editorStatusPrintMode:
        CALL bios_printString
        CALL disableInverse
editorStatusLanguageCheck:
        CALL editorStatusLanguage
editorStatusRestore:
        POP HL
        LD (bios_vars.cursorY),HL
        POP BC
        POP DE
        POP HL
        COLOR COLOR_TEXT
        RET
editorStatusLanguage:
        CALL editorReadLanguage
        LD B,A
        LD A,(editorLastLanguage)
        CP B
        RET Z
        LD A,B
        LD (editorLastLanguage),A
        LD A,(editorStatusLangPos)
        LD C,A
        CALL editorStatusPosition
        COLOR COLOR_STATUSBAR
        CALL enableInverse
        LD HL,editorStatusLat
        LD A,(editorLastLanguage)
        OR A
        JP Z,editorStatusPrintLanguage
        LD HL,editorStatusRus
editorStatusPrintLanguage:
        CALL bios_printString
        JP disableInverse

;----------------------------------------------------------------------------
; MXOS
;
; Получить бит раскладки клавиатуры из состояния BIOS.
;
; вход:
;   bios_vars.keyLocks содержит выбранные переключатели клавиатуры.
;
; выход:
;   a = значение бита 0, z = 1 если он сброшен, cf = 0. bc, de и hl сохранены.
;----------------------------------------------------------------------------

editorReadLanguage:
        LD A,(bios_vars.keyLocks)
        AND 1
        RET
editorStatusNumber:
        PUSH BC
        EX DE,HL
        XOR A
        CALL editorFormatFour
        POP BC
editorStatusDigits:
        LD HL,editorNumberBuffer+2
editorStatusTrimDigits:
        LD A,(HL)
        CP ' '
        JP NZ,editorStatusDigitsReady
        INC HL
        JP editorStatusTrimDigits
editorStatusDigitsReady:
        LD A,(editorStatusNumBytes)
        LD D,A
; C = starting text column, D = field width in screen bytes, HL = text.
editorStatusField:
        PUSH HL
        CALL editorStatusPosition
        LD A,H
        AND 0FCh
        PUSH AF
        RRCA
        RRCA
        ADD A,090h
        LD H,A
        LD L,1
        LD B,D
        LD C,8
        PUSH DE
        COLOR COLOR_HELP_F
        XOR A
        CALL uiFillRectBytes
        POP DE
        POP AF
        LD B,A
        LD A,D
        ADD A,A
        ADD A,A
        LD C,A
        LD A,(FONT_ENGINE_HEADER+5)
        LD D,A
        LD E,8
        POP HL
        LD A,UI_ALIGN_CENTER
        CALL uiAlignText
        EX DE,HL
        LD (bios_vars.cursorY),HL
        EX DE,HL
        JP bios_printStringRawZ
editorFormatFour:
        CALL editorFormatColumn
        LD A,(editorNumberBuffer)
        CP 32
        JP NZ,editorNumberOverflow
        LD A,(editorNumberBuffer+1)
        CP 32
        RET Z
editorNumberOverflow:
        LD HL,editorNumberBuffer+2
        LD B,4
editorNumberHashes:
        LD (HL),'#'
        INC HL
        DEC B
        JP NZ,editorNumberHashes
        RET
editorStatusPosition:
        LD A,(v_charStep)
        LD B,A
        XOR A
        INC C
editorStatusMultiply:
        DEC C
        JP Z,editorStatusXY
        ADD A,B
        JP editorStatusMultiply
editorStatusXY:
        LD H,A
        LD L,8
        LD (bios_vars.cursorY),HL
        RET
editorStatusLnText:     DB 'Ln',0
editorStatusColText:    DB 'Col',0
editorStatusTabText:    DB 'TAB',0
editorEncodingLabel:    DB 'KOI'
editorEncodingDigit:    DB '8','-R',0
editorStatusTabValue:   DB '4',0
editorStatusNumBytes:   DB 5
editorStatusTabBytes:   DB 2
editorStatusLat:        DB 'LAT',0
editorStatusRus:        DB 'RUS',0
editorStatusColPos:     DB 16
editorStatusTabPos:     DB 34
editorStatusModePos:    DB 47
editorStatusLangPos:    DB 61
editorLastLine:         DW 0FFFFh
editorLastCol:          DW 0FFFFh
editorLastColHi:        DB 255
editorLastMode:         DB 2
editorLastTab:          DB 255
editorLastLanguage:     DB 255

; The public input hook updates RUS/LAT even when the modifier itself does
; not return from getch. It lives only during the main editor key wait.
editorPreviousHook: DW 0
editorGetch:
        CALL editorUpdateToolbarKeys
        LD HL,editorKeyboardIdle
        CALL bios_setInputHook
        LD (editorPreviousHook),HL
        ; Use the document caret only for this wait. Modal fields and the
        ; calling shell keep the original BIOS cursor configuration.
        LD A,(bios_upper.cursorCfg)
        PUSH AF
        AND 08Fh
        JP Z,editorWaitKey
        JP M,editorWaitKey
        LD A,(v_insertMode)
        OR A
        LD A,028h              ; OVR: full eight-scanline glyph cell
        JP Z,editorSetCaret
        LD A,021h              ; INS: single scanline at the glyph baseline
editorSetCaret:
        LD (bios_upper.cursorCfg),A
editorWaitKey:
        CALL bios_getch
        PUSH AF
        POP HL
        EX (SP),HL
        LD A,H
        LD (bios_upper.cursorCfg),A
        LD HL,(editorPreviousHook)
        CALL bios_setInputHook
        POP AF
        RET
editorKeyboardIdle:
        PUSH AF
        PUSH BC
        PUSH DE
        PUSH HL
        LD HL,(bios_vars.cursorY)
        PUSH HL
        LD HL,(bios_vars.inverse)
        PUSH HL
        CALL editorUpdateToolbarKeys
        LD A,(hexActive)
        OR A
        CALL Z,editorStatusLanguage
        POP HL
        LD (bios_vars.inverse),HL
        POP HL
        LD (bios_vars.cursorY),HL
        COLOR COLOR_TEXT
        POP HL
        POP DE
        POP BC
        POP AF
        RET

editorGeometry:
        LD A,(FONT_ENGINE_HEADER+5)
        OR A
        RRA
        LD (v_charStep),A
        LD C,A
        LD B,0
        LD A,192
editorColumnsLoop:
        INC B
        SUB C
        JP NZ,editorColumnsLoop
        LD A,B
        LD (editorScreenColumns),A
        LD (v_columns),A
        RET

; В документе хранится столбец, в BIOS передаётся только экранная координата.
editorPositionCursor:
        CALL editorClampToText
        CALL editorMeasureCursor
        LD B,A
        LD A,(v_leftColumnHi)
        LD C,A
        LD A,B
        CP C
        JP C,editorWindowLeft
        PUSH HL
        LD HL,(v_leftColumn)
        EX DE,HL
        POP HL
        LD A,B
        CP C
        JP NZ,editorWindowDistance
        CALL compareHLDE
        JP C,editorWindowLeft
editorWindowDistance:
        CALL bios_sub_hl_de
        LD A,B
        SBC A,C
        OR A
        JP NZ,editorWindowRight
        LD A,(v_columns)
        LD C,A
        LD A,H
        OR A
        JP NZ,editorWindowRight
        LD A,L
        CP C
        JP C,editorCursorX
editorWindowRight:
        LD HL,(v_visualColumn)
        LD A,(v_columns)
        DEC A
        LD E,A
        LD D,0
        CALL bios_sub_hl_de
        LD A,(v_visualColumnHi)
        SBC A,0
        LD B,A
        LD DE,7
        ADD HL,DE
        LD A,B
        ADC A,0
        JP editorWindowChanged
editorWindowLeft:
        LD HL,(v_visualColumn)
        LD A,(v_visualColumnHi)
editorWindowChanged:
        LD B,A
        LD A,L
        AND 0F8h
        LD L,A
        PUSH HL
        LD A,(v_leftColumnHi)
        LD C,A
        LD A,B
        LD (v_leftColumnHi),A
        LD HL,(v_leftColumn)
        EX DE,HL
        POP HL
        LD (v_leftColumn),HL
        CALL bios_sub_hl_de
        LD A,B
        SBC A,C
        OR A
        JP Z,editorWindowForward
        CP 255
        JP NZ,editorWindowFull
        LD A,H
        CP 255
        JP NZ,editorWindowFull
        LD A,L
        CP 248
        JP NZ,editorWindowFull
        JP editorWindowShift
editorWindowForward:
        LD A,H
        OR A
        JP NZ,editorWindowFull
        LD A,L
        CP 8
        JP NZ,editorWindowFull
editorWindowShift:
        CALL editorShiftWindow
        JP editorPositionCursor
editorWindowFull:
        CALL redrawPreservingCursor
        JP editorPositionCursor
editorCursorX:
        LD C,L
        LD A,(v_charStep)
        LD B,A
        XOR A
        INC C
editorCursorMultiply:
        DEC C
        JP Z,editorCursorReady
        ADD A,B
        JP editorCursorMultiply
editorCursorReady:
        LD (bios_vars.cursorY+1),A
        RET

; HL = new left column - old left column. Large jumps render the full page.
editorShiftWindow:
        LD A,H
        OR A
        JP NZ,editorShiftRightTest
        LD A,L
        CP 8
        JP NZ,redrawPreservingCursor
        CALL saveCursor
        LD A,(v_columns)
        SUB 8
        LD (editorSliceStart),A
        LD A,(v_charStep)
        ADD A,A
        LD E,A
        ADD A,090h
        LD H,A
        LD L,EDITOR_TEXT_TOP
        LD BC,09000h+EDITOR_TEXT_TOP
        LD A,(editorCopyWidth)
        SUB E
        LD D,A
        ADD A,090h
        LD (editorStripX),A
        JP editorShiftCopy
editorShiftRightTest:
        INC HL
        LD A,H
        CP 255
        JP NZ,redrawPreservingCursor
        LD A,L
        CP 249
        JP NZ,redrawPreservingCursor
        CALL saveCursor
        XOR A
        LD (editorSliceStart),A
        LD A,090h
        LD (editorStripX),A
        LD A,(v_charStep)
        ADD A,A
        LD E,A
        ADD A,090h
        LD B,A
        LD C,EDITOR_TEXT_TOP
        LD HL,09000h+EDITOR_TEXT_TOP
        LD A,(editorCopyWidth)
        SUB E
        LD D,A
editorShiftCopy:
        COLOR COLOR_TEXT
        LD E,EDITOR_HEIGHT
        CALL uiCopyTextRect
        ; The 62nd 6px glyph ends four pixels before the scrollbar.
        ; A byte-aligned shift can bring old glyph bits into that gap.
        LD A,(editorCopyWidth)
        CP 47
        JP NZ,editorShiftGapReady
        LD A,(v_charStep)
        CP 3
        JP NZ,editorShiftGapReady
        LD HL,372
        LD BC,4
        LD DE,EDITOR_TEXT_TOP*256+EDITOR_HEIGHT
        XOR A
        CALL uiFillRectPixels
editorShiftGapReady:
        CALL editorRefreshLengths
        LD A,(editorStripX)
        LD H,A
        LD L,EDITOR_TEXT_TOP
        LD A,(v_charStep)
        ADD A,A
        LD B,A
        LD C,EDITOR_HEIGHT
        XOR A
        CALL uiFillRectBytes
        LD A,8
        LD (editorSliceWidth),A
        LD A,EDITOR_FIRST_Y
        LD (bios_vars.cursorY),A
        LD HL,(v_topLine)
        LD B,EDITOR_ROWS
editorShiftDraw:
        CALL drawTextSlice
        OR A
        JP Z,restoreCursor
        LD A,(bios_vars.cursorY)
        ADD A,10
        LD (bios_vars.cursorY),A
        DEC B
        JP NZ,editorShiftDraw
        JP restoreCursor
editorStripX: DB 0

; Снимает собственный адрес возврата, сбрасывает SP сеанса и прыгает по нему.
; Вход: корректный операнд smc_resetSessionStack. Выход: косвенный переход через HL.
; HL разрушен; AF, BC, DE и флаги сохраняются. Это не обычный RET.
resetSessionStack:  POP   HL

        ; Установка указателя стека, агрумент этой инструкции
        ; может меняться из п/п beginSession
smc_resetSessionStack:   LD    SP,8F50H
        ; Переход по адресу в hl
        JP    (HL)

editorIsWordSpace:
        CP 9
        RET Z
        CP 32
        RET
moveNextWord:
        CALL editorResetGoal
        CALL editorClampToText
        CALL getLineBufferCursor
editorWordRight:
        CALL gapTextRead
        OR A
        RET Z
        CP 13
        JP Z,editorWordNextLine
        CALL editorIsWordSpace
        JP Z,editorWordSpaces
        INC HL
        CALL editorIncrementColumn
        JP editorWordRight
editorWordSpaces:
        CALL gapTextRead
        CP 13
        JP Z,editorWordNextLine
        CALL editorIsWordSpace
        RET NZ
        INC HL
        CALL editorIncrementColumn
        JP editorWordSpaces
editorWordNextLine:
        LD HL,(v_currentLine)
        PUSH HL
        CALL keyHandler_Down
        POP DE
        LD HL,(v_currentLine)
        CALL compareHLDE
        RET Z
        CALL keyHandler_Home
        CALL getLineBufferCursor
        JP editorWordSpaces

;----------------------------------------------------------------------------
; MXOS
;
; Продвинуть логическую колонку редактора на одну позицию.
;
; вход:
;   v_column = текущая 16-битная колонка.
;
; выход:
;   v_column увеличена на единицу по модулю 65536. Все регистры и флаги сохранены.
;----------------------------------------------------------------------------

editorIncrementColumn:
        PUSH HL
        LD HL,(v_column)
        INC HL
        LD (v_column),HL
        POP HL
        RET
movePreviousWord:
        CALL editorResetGoal
        CALL editorClampToText
        CALL getLineBufferCursor
        LD HL,(v_column)
        LD A,H
        OR L
        JP Z,editorWordPreviousLine
        DEC HL
        LD (v_column),HL
        CALL getLineBufferCursor
editorWordLeftSpaces:
        CALL gapTextRead
        CALL editorIsWordSpace
        JP NZ,editorWordLeft
        CALL editorPreviousColumn
        JP Z,editorWordPreviousLine
        DEC HL
        JP editorWordLeftSpaces
editorWordLeft:
        CALL editorPreviousColumn
        RET Z
        DEC HL
        CALL gapTextRead
        CALL editorIsWordSpace
        JP NZ,editorWordLeft
        JP editorIncrementColumn
editorWordPreviousLine:
        LD HL,(v_currentLine)
        PUSH HL
        CALL keyHandler_Up
        POP DE
        LD HL,(v_currentLine)
        CALL compareHLDE
        RET Z
        CALL keyHandler_End
        JP movePreviousWord
editorPreviousColumn:
        PUSH HL
        LD HL,(v_column)
        LD A,H
        OR L
        JP Z,editorPreviousDone
        DEC HL
        LD (v_column),HL
        OR 1
editorPreviousDone:
        POP HL
        RET
clearTextViewport:
        CALL editorInvalidateBars
        CALL editorResetLengths
        PUSH HL
        PUSH DE
        PUSH BC
        COLOR COLOR_TEXT
        LD HL,09000h+EDITOR_TEXT_TOP
        LD B,48
        LD C,EDITOR_HEIGHT
        XOR A
        CALL uiFillRectBytes
        POP BC
        POP DE
        POP HL
        RET
enableInverse:  PUSH  HL            ; [5]
        LD    HL,0FFFFH   ; 65535
setVideoInverse: LD    (bios_vars.inverse),HL
        POP   HL
        RET
disableInverse:  PUSH  HL            ; [6]
        LD    HL,0000H
        JP    setVideoInverse

;----------------------------------------------------------------------
; Запрос имени файла в строке состояния


editorRefreshLengths:
        LD HL,(v_topLine)
        LD BC,editorRowLengths
        LD A,EDITOR_ROWS
editorRefreshRow:
        PUSH AF
        PUSH BC
        CALL editorRowSelection
        LD A,(v_columns)
        LD C,A
        XOR A
        CALL editorPrepareRow
        LD A,(v_columns)
        SUB C
        LD D,A
        LD A,(editorSelectionRight)
        CP D
        JP C,editorRefreshLengthReady
        LD D,A
editorRefreshLengthReady:
        CALL nextLine
        POP BC
        LD A,D
        LD (BC),A
        INC BC
        POP AF
        DEC A
        JP NZ,editorRefreshRow
        RET
