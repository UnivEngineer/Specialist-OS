; Старт, данные сеанса и цикл строки состояния
; Порядок включения задан главным .asm; см. README.md.

smc_entryDispatch:
        JP    openCommandLineFile    ; editorStart заменяет адрес на beginSession
        JP    resumeAtTextPointer

        ; Текст после образа программы; исключительная граница берётся из memtop.
v_textBeginLo:  DB    EDITOR_TEXT_BEGIN & 255
v_textBeginHi:  DB    EDITOR_TEXT_BEGIN >> 8
v_textLimitLo:  DB    0
v_textLimitHi:  DB    APP_MEMORY_END >> 8

editorStart:
        LD HL,beginSession
        LD (smc_entryDispatch+1),HL
        JP beginSession
v_documentDirty: DB 0
editorInterfaceReady: DB 0
v_column: DW 0                 ; Байтовый индекс курсора в строке, с нуля.
v_leftColumn: DW 0             ; Визуальная колонка левого края, младшие 16 бит.
v_leftColumnHi: DB 0
v_columns: DB 64
v_charStep: DB 3
v_undoStart: DW 0
v_undoLength: DW 0
v_undoColumn: DW 0
v_undoLine: DW 0
v_undoLineNumber: DW 0
v_undoLeftColumn: DW 0
v_undoLeftColumnHi: DB 0
v_topLine:   DW    0000H ; первая строка видимой страницы
v_textEnd:   DW    0000H ; логическая исключительная граница файла
v_currentLine:   DW    0000H ; начало строки курсора в документе
v_lineDirty:   DB    00H ; есть снимок текущей строки для F8
v_insertMode:   DB    0FFH ; 0 = замена, 0FFh = вставка
v_selectionBegin:   DW    0000H
v_selectionEnd:   DW    0000H ; исключающая граница выделенных байтов
v_lineNumber:   DW    0000H
v_searchText:  DS MAX_PATH,0

txtIns:     DB    "INSERT",00H
txtOvr:     DB    "OVR   ",00H

beginSession:   ; Читаем указатель стека в hl
        LD    HL,0000H
        ADD   HL,SP
        ; САМОМОДИФИКАЦИЯ: SP сеанса хранится в операнде LD SP,nn.
        LD    (smc_resetSessionStack+1),HL

        COLOR COLOR_TEXT

; Вход: границы текста в v_textBeginLo/v_textLimitLo. Выход: цикл редактора.
; Разрушает AF, BC, DE, HL, флаги; последний доступный байт резервируется под конец.
; v_textEnd задаёт исключительную границу сырых байтов после загрузки.
scanDocument:
        CALL editorPrepareDocument
        LD A,(editorPendingHex)
        OR A
        JP Z,clearAndRedrawDocument
        CALL editorSetScrollLayout
        CALL editorInvalidateBars
        CALL editorResetLengths
        CALL hexEnter
        JP NC,editorStatusLoop
clearAndRedrawDocument:
        CALL editorPaintDocument
editorStatusLoop:
        CALL hexCheckTextCaret
        CALL editorApplyScrollLayout
        CALL editorUpdateMark
        CALL editorPositionCursor
        CALL updateStatusBar
        CALL editorDrawDocumentBars
        LD HL,editorStatusLoop
        PUSH HL
        JP keyInput

editorPrepareDocument:
        XOR A
        LD (hexActive),A
        LD (hexRestoreLF),A
        LD HL,0
        LD (hexTop),HL
        LD HL,(v_textEnd)
        EX DE,HL
        LD HL,(v_textBeginLo)
        CALL editorCountSeparators
        INC BC
        LD H,B
        LD L,C
        LD (editorBarV.total),HL
        CALL editorGeometry
        LD HL,0
        LD (v_column),HL
        LD (v_leftColumn),HL
        LD    A,10H         ; 16
        LD    (editor_vars.last),A
        XOR   A
        LD    (v_lineDirty),A
        LD (editorMarkActive),A
        LD (v_leftColumnHi),A
        LD (v_goalValid),A
        LD    H,A
        LD    L,A
        LD    (v_selectionBegin),HL
        LD    (bios_vars.inverse),HL
        LD    HL,0001H    ; 1
        LD    (v_lineNumber),HL
        LD    HL,(v_textBeginLo)
        LD    (v_topLine),HL
        LD    (v_currentLine),HL
        JP gapInitialize

editorPaintDocument:
        CALL editorSetScrollLayout
        CALL editorInvalidateBars
        ; Paint every pixel in the editor's palette, including both margins.
        COLOR COLOR_TEXT
        LD HL,09000h
        LD B,48
        LD C,247
        XOR A
        CALL uiFillRectBytes
        CALL editorResetLengths
        LD A,1
        LD (editorInterfaceReady),A
        JP    synchronizeTopAndRedraw
