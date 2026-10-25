; Старт, данные сеанса и цикл строки состояния
; Порядок включения задан главным .asm; см. README.md.

smc_entryDispatch:
        JP    openCommandLineFile    ; адрес этого перехода изменяется п/п restartAfterLoad
        JP    resumeAtTextPointer

        ; Две пары байтов: начало 01800h и верхняя граница 08500h.
v_textBeginLo:  DB    0
v_textBeginHi:  DB    18h
v_textLimitLo:  DB    0
v_textLimitHi:  DB    85h

; САМОМОДИФИКАЦИЯ: первые 64 байта отсюда становятся буфером строки.
; Внутри лежат restartAfterLoad и smc_sessionTarget; повторный вызов опасен.
startupBannerAndLineBuffer:   LD    HL,txtBanner
        CALL  bios_printString

        ; Возврат F4/F6. Эта область уничтожается при beginLineEdit.
restartAfterLoad:   ; Контрольная сумма 07000h-08F00h; результат далее не используется.
        LD    HL,7000H
        LD    DE,8F00H
        CALL  bios_calcCS
        ; САМОМОДИФИКАЦИЯ: операнд JP в начале редактора.
        ; Меняем адрес в первой инстукции JP в начале редактора на beginSession
smc_sessionTarget:   LD    HL, beginSession
        LD    (smc_entryDispatch+1),HL

        ; И прыгаем на beginSession
        JP    beginSession

txtBanner:   DB    0AH,"          EDITOR VERSION 4.1, (C) OM"
lineJoinLimit:   DB    "S"
lineWordRightLimit:   DB    "K"
lineBufferLast:   DB    " "
lineBufferEnd:   DB    "KSOFT ",27H,"92",2EH,00H
v_topLine:   DW    0000H ; первая строка видимой страницы
v_textEnd:   DW    0000H ; адрес маркера 0FFh
v_currentLine:   DW    0000H ; начало строки курсора в документе
v_lineDirty:   DB    00H ; строка изменяется в буфере кода заставки
v_insertMode:   DB    00H ; 0 = замена, 0FFh = вставка
v_selectionBegin:   DW    0000H
v_selectionEnd:   DW    0000H ; первый байт после выделенных строк
v_lineNumber:   DW    0000H
t_tabSteps:   DB    04H,05H,08H ; позиции 4, 9, 17, 25... (с нуля)
v_tapeMode:   DB    00H ; 0 = загрузка, 1 = добавление, 0FFh = проверка
txtOutOfMemory:   DB    1FH,0AH,"  OUT OF MEMORY",2EH,00H
txtTapePrompt:   DB    1FH
txtTapeFile:  DB    0AH," FILE: ",00H
txtEraseChar:  DB    08H,20H,08H,00H           ; <-, ' ', <-
txtTapeError:  DB    0AH," MISTAKE",2EH,00H
txtSearchPrompt:  DB    "SEARCH: "
v_searchText:  DB    00H,"ALL V33",00H,"RLN]N",00H,"y"

txtNew:     DB    1FH,0AH," NEW ?",00H
txtIns:     DB    "INSERT   ",00H
txtOvr:     DB    "OVERWRITE",00H
txtLine:    DB    "LINE",00H
txtCol:     DB    "COL",00H

beginSession:   ; Читаем указатель стека в hl
        LD    HL,0000H
        ADD   HL,SP
        ; САМОМОДИФИКАЦИЯ: SP сеанса хранится в операнде LD SP,nn.
        LD    (smc_resetSessionStack+1),HL

        COLOR COLOR_TEXT

; Вход: границы текста в v_textBeginLo/v_textLimitLo. Выход: цикл редактора.
; Разрушает AF, BC, DE, HL, флаги; сканирование не проверяет верхнюю границу.
; Нулевой байт превращается в маркер 0FFh, остальные данные сохраняются.
scanDocument:   LD    A,10H         ; 16
        LD    (editor_vars.last),A
        XOR   A
        LD    (v_lineDirty),A
        LD    H,A
        LD    L,A
        LD    (v_selectionBegin),HL
        LD    (bios_vars.inverse),HL
        LD    HL,0001H    ; 1
        LD    (v_lineNumber),HL
        LD    HL,(v_textBeginLo)
        LD    (v_topLine),HL
        LD    (v_currentLine),HL
        LD    E,L
        LD    D,H
scanDocumentByte:   LD    A,(HL)
        OR    A
        JP    NZ,testDocumentEnd
        DEC   A
        LD    (HL),A
testDocumentEnd:   INC   A
        JP    Z,ensureNonemptyDocument
        INC   HL
        JP    scanDocumentByte
ensureNonemptyDocument:   CALL  compareHLDE
        JP    NZ,rememberDocumentEnd
        LD    (HL),0DH    ; 13
        INC   HL
        LD    (HL),0FFH   ; 255
rememberDocumentEnd:   LD    (v_textEnd),HL
clearAndRedrawDocument:  LD    C,1FH         ; 31
        CALL  bios_printChar
        CALL  synchronizeTopAndRedraw
editorStatusLoop:  LD    HL,editorStatusLoop    ; 53535
        PUSH  HL
        LD    HL,(bios_vars.cursorY)
        PUSH  HL
        CALL  enableInverse
        LD    HL,0000H
        CALL  addCursorColumn
        EX    DE,HL
        LD    HL,8CF9H    ; 36089
        LD    (bios_vars.cursorY),HL
        EX    DE,HL
        LD    DE,0FFF6H   ; 65526
        LD    B,20H         ; 32 ' '
        INC   HL
        CALL  printDecimalDigit
        LD    A,L
        ADD   A,3AH         ; 58 ':'
        LD    C,A
        CALL  bios_printChar
        CALL  disableInverse
        POP   HL
        LD    (bios_vars.cursorY),HL
        JP    keyInput

;----------------------------------------------------------------------

        ; Продолжение разбора кода нажатой клавиши
