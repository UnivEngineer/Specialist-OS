;----------------------------------------------------------------------------
; MXOS
; E.COM - текстовой редактор
;
; 2022-02-03 Дизассемблировано SpaceEngineer
;----------------------------------------------------------------------------

    INCLUDE "../include/mxos.inc"

; Используемые подпрограммы DOS.SYS:
; bios_beep_Old         = 0C170h
; bios_delay_b          = 0C190h
; bios_cmp_hl_de        = 0C427h
; bios_memcpy_bc_hl     = 0C42Dh
; bios_reboot           = 0C800h
; bios_getch            = 0C803h
; bios_tapeRead         = 0C806h
; bios_printChar        = 0C809h
; bios_tapeWrite        = 0C80Ch
; bios_printString      = 0C818h
; bios_calcCS           = 0C82Ah
; bios_fileCreate       = 0C845h
; bios_fileLoadInfo     = 0C851h
; bios_fileNamePrepare  = 0C85Ah
; bios_fileLoad2        = 0C866h

; Используемые переменные DOS.SYS:
; bios_vars.lastKey     = 08FF0h
; bios_vars.tapeInverse = 08FF3h
; bios_vars.cursorDelay = 08FF4h
; bios_vars.inverse     = 08FFAh
; bios_vars.cursorY     = 08FFCh

; Используемые порты устройств:
; IO_KEYB_A = 0FFE0h
; IO_KEYB_B = 0FFE1h

; Собственные переменные
    STRUCT EDITOR_VARIABLES
buffer  BLOCK  10, 0
last    BLOCK  1,  0
    ENDS

; Блок переменных редактора начинается с адреса 8F80H
editor_vars EDITOR_VARIABLES = 8F80H

; Начало программы
    IF NEW_MEMORY_MAP==0
        ORG 0E800h
    ELSE
        ORG 0D000h
    ENDIF

; Буферы в конце программы
FILE_NAME_RECALL_END = $ + 0F0Ah ; 0DF0AH
FILE_NAME_HISTORY = $ + 0F00h ; 0DF00H

smc_entryDispatch:
        JP    openCommandLineFile    ; адрес этого перехода изменяется п/п restartAfterLoad
        JP    resumeAtTextPointer

        ; Что-то непонятное, границы буфера текста?
v_textBeginLo:  DB    0
v_textBeginHi:  DB    18h
v_textLimitLo:  DB    0
v_textLimitHi:  DB    85h

startupBannerAndLineBuffer:   LD    HL,txtBanner
        CALL  bios_printString

        ; Похоже на обработчик ошибки ввода/вывода с ленты
restartAfterLoad:   ; ??? Считаем контрольную сумму блока 7000-8EFF
        LD    HL,7000H
        LD    DE,8F00H
        CALL  bios_calcCS
        ; Суровая самомодификация кода:
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
v_topLine:   DW    0000H
v_textEnd:   DW    0000H
v_currentLine:   DW    0000H
v_lineDirty:   DB    00H
v_insertMode:   DB    00H
v_selectionBegin:   DW    0000H
v_selectionEnd:   DW    0000H
v_lineNumber:   DW    0000H
t_tabSteps:   DB    04H,05H,08H
v_tapeMode:   DB    00H
txtOutOfMemory:   DB    1FH,0AH,"  OUT OF MEMORY",2EH,00H
txtTapePrompt:   DB    1FH
txtTapeFile:  DB    0AH," FILE: ",00H
txtEraseChar:  DB    08H,20H,08H,00H           ; <-, ' ', <-
txtTapeError:  DB    0AH," MISTAKE",2EH,00H
txtSearchPrompt:  DB    "SEARCH: "
v_searchText:  DB    00H,"ALL V33",00H,"RLN]N",00H,"y"
txtNew:  DB    1FH,0AH," NEW ?",00H
txtIns:  DB    "INSERT   ",00H
txtOvr:  DB    "OVERWRITE",00H
txtLine:  DB    "LINE",00H
txtCol:  DB    "COL",00H

beginSession:   ; Читаем указатель стека в hl
        LD    HL,0000H
        ADD   HL,SP
        ; И записываем его в код п/п smc_resetSessionStack
        LD    (smc_resetSessionStack+1),HL

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
keyInput2:
        LD    C,A               ; сохраняем в регистре c
        CP    7FH               ; Del - удалить символ над курсором
        JP    Z,keyHandler_Del
        CP    07H               ; F8 - отмена изменений в строке
        JP    Z,keyHandler_F8
        CP    01H               ; F2 - курсор на страницу вверх (PageUp)
        JP    Z,keyHandler_F2
        CP    02H               ; F3 - курсор на страницу вниз (PageDn)
        JP    Z,keyHandler_F3
        CP    20H               ; пробел
        JP    NC,keyHandler_Space
        OR    A                 ; F1 - Ins (перекл. режима Insert/Overwrite)
        JP    Z,keyHandler_F1

        ; Теперь проходм по таблице подпрограмм
        LD    HL, KeyHendlerTable
fkh_Loop:
        LD    A,(HL)        ; код клавиши
        OR    A             ; если 0 - достигнут конец таблицы
        JP    Z,Beep4       ; бип 4 раза и выход
        INC   HL
        LD    E,(HL)        ; de = адрес подпрограммы
        INC   HL
        LD    D,(HL)
        INC   HL
        CP    C             ; сравниваем a и запомненный в с код клавиши
        JP    NZ,fkh_Loop   ; если не равно, повтор цикла
        PUSH  DE            ; алрес п/п - в стек
        RET                 ; переход на адрес п/п

;----------------------------------------------------------------------

    STRUCT  KEY_HANDLER
key:        DB  0
address:    DW  0
    ENDS

        ; Таблица переходов на подпрограммы
        ; код символа, адрес
KeyHendlerTable:
        KEY_HANDLER  1Ah, keyHandler_Down   ; Вниз      - курсор вниз
        KEY_HANDLER  19h, keyHandler_Up     ; Вверх     - курсор вверх
        KEY_HANDLER  08h, keyHandler_Left   ; Влево     - курсор влево
        KEY_HANDLER  18h, keyHandler_Right  ; Вправо    - курсор вправо
        KEY_HANDLER  0Dh, keyHandler_Enter  ; Enter     - вставить строку ниже (не разбивает текущую строку!)
        KEY_HANDLER  09h, keyHandler_Tab    ; Tab       - курсор на 4 (8) позиций влево, с привязкой по сетке (?)
        KEY_HANDLER  1Fh, keyHandler_CTP    ; СТР       - выход без сохранения!
        KEY_HANDLER  02h, keyHandler_F3_2   ; F3 снова?
        KEY_HANDLER  06h, keyHandler_F7     ; F7        - поиск текста в строке курсора и ниже (перематывает документ на строку с текстом)
        KEY_HANDLER  0Ch, keyHandler_Home   ; Home      - курсор в начало строки
        KEY_HANDLER  0Ah, keyHandler_End    ; End (ПС)  - курсор в конец строки
        KEY_HANDLER  1Bh, keyHandler_Esc    ; Esc       - режим Esc-последовательности
        DB  00H

;----------------------------------------------------------------------

        ; Обработка нажатия Esc
keyHandler_Esc:
        CALL    commitLineEdit

        ; Ожидаем ввод второго символа Esc-последовательности
        CALL    bios_getch
        LD      C,A

        ; Проходм по второй таблице подпрограмм
        LD      HL,KeyHendlerTable2
        JP      fkh_Loop

;----------------------------------------------------------------------

        ; Таблица переходов на подпрограммы
        ; код символа, адрес
KeyHendlerTable2:
        KEY_HANDLER  0Ch, keyHandler_Esc_Home   ; Esc-Home     - курсор в начало первой страницы
        KEY_HANDLER  0Ah, keyHandler_Esc_End    ; Esc-End (ПС) - курсор в начало последней страницы
        KEY_HANDLER  4Ch, keyHandler_Esc_L      ; Esc-L - выделить строки текста, второй раз Esc-L - конец выделения
        KEY_HANDLER  44h, keyHandler_Esc_D      ; Esc-D - удалить выделенные строки
        KEY_HANDLER  55h, keyHandler_Esc_U      ; Esc-U - снять выделение строк
        KEY_HANDLER  4Fh, keyHandler_Esc_O      ; Esc-O - сохранить файл на магнитофон
        KEY_HANDLER  49h, keyHandler_Esc_I      ; Esc-I - загрузить файл с магнитофона
        KEY_HANDLER  56h, keyHandler_Esc_V      ; Esc-V - загрузить файл с магнитофона (в другом формате?)
        KEY_HANDLER  47h, keyHandler_Esc_G      ; Esc-G - загрузить файл с магнитофона и вставить в конец документа
        KEY_HANDLER  4Eh, keyHandler_Esc_N      ; Esc-N - новый файл (очистить буфер)
        KEY_HANDLER  4Ah, keyHandler_Esc_J      ; Esc-J - объединить строки в положении курсора и ниже
        KEY_HANDLER  53h, keyHandler_Esc_S      ; Esc-S - разбить строку в положении курсора
        KEY_HANDLER  43h, keyHandler_Esc_C      ; Esc-C - вставить выделенные строки ниже положения курсора
        KEY_HANDLER  4Dh, keyHandler_Esc_M      ; Esc-M - переместить выделенные строки ниже положения курсора
        DB  00H

;----------------------------------------------------------------------

keyHandler_CTP:
        LD    C,1FH         ; 31
        CALL  bios_printChar
        CALL  commitLineEdit
        CALL  resetSessionStack
        JP    bios_reboot

keyHandler_F3_2:
        CALL  commitLineEdit
        LD    C,1FH         ; 31
        CALL  bios_printChar
        CALL  resetSessionStack
        RET

keyHandler_Esc_Home:
        LD    HL,0001H    ; 1; [4]
        LD    (v_lineNumber),HL
        LD    HL,(v_textBeginLo)
        LD    (v_topLine),HL
        LD    (v_currentLine),HL
synchronizeTopAndRedraw:   LD    HL,(v_topLine)   ; [10]
        EX    DE,HL
        LD    HL,(v_currentLine)
        CALL  compareHLDE
        JP    Z,clearAndDrawPage
        CALL  previousCurrentLine
        JP    synchronizeTopAndRedraw
clearAndDrawPage:   CALL  clearTextViewport         ; [2]
drawPage:   LD    BC,180CH    ; 6156; [2]
drawPageLine:  CALL  bios_printChar
        CALL  drawTextLine
        INC   A
        JP    Z,finishPageAndHome
        DEC   B
        LD    C,0AH         ; 10
        JP    NZ,drawPageLine
finishPageAndHome:   CALL  drawStatusBar         ; [2]
        LD    C,0CH         ; 12
        JP    bios_printChar
commitAndRedraw:  CALL  commitLineEdit         ; [3]
redrawPreservingCursor:   CALL  saveCursor         ; [5]
        LD    HL,(v_topLine)
        CALL  clearAndDrawPage
        JP    restoreCursor

keyHandler_Down:
        CALL  commitLineEdit
        CALL  nextCurrentLine
        RET   Z
        CALL  updateLineNumber
        LD    A,(bios_vars.cursorY)
        CP    0EEH          ; 238
        JP    NZ,bios_printChar
        LD    HL,(v_topLine)
        CALL  nextLine
        LD    (v_topLine),HL
        CALL  saveCursor
        CALL  scrollTextUp
        LD    HL,(v_currentLine)
        CALL  drawTextLine
        JP    restoreCursor

keyHandler_Up
        CALL  commitLineEdit
        CALL  previousCurrentLine
        RET   Z
        CALL  updateLineNumber
        LD    A,(bios_vars.cursorY)
        CP    08H           ; 8
        JP    NZ,bios_printChar
        LD    HL,(v_currentLine)
        LD    (v_topLine),HL
        PUSH  HL
        CALL  insertScreenRow
        POP   HL
        CALL  saveCursor
        CALL  drawTextLine
        JP    restoreCursor
drawTextLine:   CALL  keyHandler_Home          ; [4]
        CALL  testLineSelected
        JP    NC,drawLineByte
        LD    A,0FFH                ; 255
        LD    (bios_vars.inverse),A
drawLineByte:  LD    A,(HL)
        CP    0DH           ; 13
        LD    C,20H         ; 32 ' '
        CALL  Z,bios_printChar
drawLineLoop:  LD    A,(HL)
        LD    C,A
        INC   A
        JP    Z,finishDrawLine
        INC   HL
        CALL  bios_printChar
        CP    0EH           ; 14
        JP    NZ,drawLineLoop
finishDrawLine:  XOR   A
        LD    (bios_vars.inverse),A
        LD    (bios_vars.inverse+1),A
        LD    A,C
        RET
testLineSelected: PUSH  DE            ; [7]
        EX    DE,HL
        LD    HL,(v_selectionBegin)
        LD    A,H
        OR    L
        JP    Z,selectionTestDone
        DEC   HL
        CALL  compareHLDE
        JP    NC,selectionTestDone
        LD    HL,(v_selectionEnd)
        DEC   HL
        CALL  compareHLDE
        CCF
selectionTestDone:  EX    DE,HL         ; [2]
        POP   DE
        RET

keyHandler_Right:
        CALL  testShiftKey
        JP    Z,moveNextWord
moveRightWithinLine:  LD    B,0BAH                ; 186
        JP    moveHorizontalWithinLine

keyHandler_Left:
        CALL  testShiftKey
        JP    Z,movePreviousWord
        LD    B,00H
moveHorizontalWithinLine:  LD    A,(bios_vars.cursorY+1)
        CP    B
        RET   Z
        JP    bios_printChar

keyHandler_End:
        CALL  beginLineEdit
        CALL  trimLineBuffer
        LD    HL,startupBannerAndLineBuffer             ; 53258
        CALL  subtractHLFromDE
        LD    A,E
        JP    clampCursorColumn

keyHandler_Home:
        XOR   A             ; [4]
        LD    (bios_vars.cursorY+1),A
        RET

keyHandler_Enter:
        LD    HL,(v_currentLine)
        CALL  testLineSelected
        JP    C,Beep4
        CALL  beginLineEdit
        CALL  trimLineBuffer
        LD    A,0DH         ; 13
        LD    (DE),A
        CALL  commitLineEdit
        CALL  keyHandler_Home
        LD    HL,(v_currentLine)
        LD    C,18H         ; 24
indentInsertedLine:  LD    A,(HL)
        CP    20H           ; 32 ' '
        JP    NZ,moveToInsertedLine
        CALL  bios_printChar
        INC   HL
        JP    indentInsertedLine
moveToInsertedLine:  LD    C,1AH         ; 26
        CALL  keyHandler_Down
        JP    insertScreenRow

        ; В lineBufferEnd изначально текст "KSOFT '92"
trimLineBuffer:  LD    DE, lineBufferEnd

        ; Пропус пробелов в строке по адресу de
SkipSpaces:
        DEC   DE
        LD    A,(DE)
        CP    20H
        JP    Z,SkipSpaces
        INC   DE
        RET

beginLineEdit:  LD    A,(v_lineDirty)    ; [7]
        OR    A
        RET   NZ
        cpl
        LD    (v_lineDirty),A
        LD    HL,startupBannerAndLineBuffer             ; 53258
        LD    A,40H         ; 64 '@'
clearLineBufferLoop:  LD    (HL),20H    ; 32 ' '
        INC   HL
        DEC   A
        JP    NZ,clearLineBufferLoop
        LD    DE,startupBannerAndLineBuffer             ; 53258
        LD    HL,(v_currentLine)
        LD    B,40H         ; 64 '@'
copyLineToBufferLoop:  LD    A,(HL)
        CP    0DH           ; 13
        RET   Z
        CP    0FFH          ; 255
        RET   Z
        DEC   B
        RET   Z
        LD    (DE),A
        INC   HL
        INC   DE
        JP    copyLineToBufferLoop

keyHandler_Space:
        LD    HL,(v_currentLine)
        CALL  testLineSelected
        JP    C,Beep4
        CALL  beginLineEdit
        CALL  getLineBufferCursor
        LD    A,(v_insertMode)
        OR    A
        JP    NZ,insertTypedCharacter
        LD    (HL),C
        CALL  bios_printChar
        LD    A,(bios_vars.cursorY+1)
        CP    0BDH          ; 189
        RET   NZ
        LD    C,08H         ; 8
        JP    bios_printChar
insertTypedCharacter:  LD    DE,lineBufferLast             ; 53321
        CALL  SkipSpaces
        PUSH  HL
        LD    HL,lineBufferLast             ; 53321
        CALL  compareHLDE
        JP    Z,drawInsertedTail
        INC   DE
drawInsertedTail:  POP   HL
        CALL  saveCursor
insertCharacterLoop:  LD    B,(HL)
        LD    (HL),C
        CALL  bios_printChar
        LD    C,B
        INC   HL
        CALL  compareHLDE
        JP    C,insertCharacterLoop
        CALL  restoreCursor
        LD    C,18H         ; 24
        JP    moveRightWithinLine

keyHandler_F1:
        LD    HL,v_insertMode             ; 53340
        LD    A,(HL)
        cpl
        LD    (HL),A
        JP    updateEditMode
getLineBufferCursor:  LD    HL,startupBannerAndLineBuffer             ; 53258; [4]
addCursorColumn:  LD    A,(bios_vars.cursorY+1)        ; [4]
        OR    A
cursorColumnLoop:  RET   Z
        INC   HL
        SUB   03H           ; 3
        JP    cursorColumnLoop

keyHandler_Del:
        CALL  beginLineEdit
        CALL  getLineBufferCursor
        PUSH  HL
        LD    C,L
        LD    B,H
        INC   BC
        LD    DE,lineBufferLast             ; 53321
        CALL  copyUntilDE
        POP   HL
        LD    DE,lineBufferLast             ; 53321
        CALL  SkipSpaces
        PUSH  HL
        LD    HL,lineBufferLast             ; 53321
        CALL  compareHLDE
        JP    Z,drawDeletedTail
        INC   DE
drawDeletedTail:  POP   HL
        CALL  saveCursor
redrawTailLoop:  LD    C,(HL)
        CALL  bios_printChar
        CALL  compareHLDE
        INC   HL
        JP    C,redrawTailLoop
        JP    restoreCursor

commitLineEdit:  LD    A,(v_lineDirty)  ; читаем v_lineDirty
        OR    A
        RET   Z         ; если v_lineDirty == 0, выходим
        XOR   A         ; инвертируем
        LD    (v_lineDirty),A  ; сохраняем обртано
        PUSH  BC
        CALL  trimLineBuffer
        LD    HL,startupBannerAndLineBuffer
        CALL  subtractHLFromDE
        LD    HL,(v_currentLine)
        EX    DE,HL
        ADD   HL,DE
        EX    DE,HL
        CALL  nextLine
        INC   A
        JP    Z,resizeEditedLine
        DEC   HL
resizeEditedLine:  CALL  resizeTextRange
        LD    HL,(v_currentLine)
        LD    BC,startupBannerAndLineBuffer             ; 53258
        CALL  compareHLDE
        CALL  NZ,copyUntilDE
        POP   BC
        RET
resizeTextRange:  CALL  compareHLDE         ; [5]
        RET   Z
        PUSH  AF
        PUSH  DE
        CALL  subtractHLFromDE
        LD    B,D
        LD    C,E
        EX    DE,HL
        LD    HL,(v_selectionBegin)
        CALL  compareHLDE
        JP    C,adjustSelectionEnd
        ADD   HL,BC
        LD    (v_selectionBegin),HL
adjustSelectionEnd:  LD    HL,(v_selectionEnd)
        EX    DE,HL
        CALL  compareHLDE
        EX    DE,HL
        JP    NC,moveDocumentTail
        ADD   HL,BC
        LD    (v_selectionEnd),HL
moveDocumentTail:  EX    DE,HL
        POP   DE
        POP   AF
        JP    C,expandDocumentTail
        PUSH  DE
        LD    C,E
        LD    B,D
        EX    DE,HL
        LD    HL,(v_textEnd)
        INC   HL
        EX    DE,HL
        CALL  subtractHLFromDE
compactTailPairs:  LD    A,E
        AND   0FEH          ; 254
        OR    D
        JP    Z,compactTailRemainder
        LD    A,(HL)
        LD    (BC),A
        INC   BC
        INC   HL
        DEC   DE
        LD    A,(HL)
        LD    (BC),A
        INC   BC
        INC   HL
        DEC   DE
        JP    compactTailPairs
compactTailRemainder:  CP    E
        JP    Z,rememberCompactedEnd
        LD    A,(HL)
        LD    (BC),A
        INC   BC
rememberCompactedEnd:  DEC   BC
        LD    L,C
        LD    H,B
        LD    (v_textEnd),HL
        POP   DE
        RET
expandDocumentTail:  PUSH  DE
        CALL  subtractHLFromDE
        LD    HL,(v_textEnd)
        LD    B,H
        LD    C,L
        ADD   HL,DE
        POP   DE
        PUSH  DE
        PUSH  HL
        EX    DE,HL
        LD    HL,(v_textLimitLo)
        CALL  compareHLDE
        JP    C,documentOutOfMemory
        POP   HL
        POP   DE
        LD    (v_textEnd),HL
        PUSH  HL
        EX    DE,HL
        CALL  subtractHLFromDE
        EX    (SP),HL
        INC   DE
expandTailQuads:  LD    A,E
        AND   0FCH          ; 252
        OR    D
        JP    Z,expandTailRemainder
        LD    A,(BC)
        LD    (HL),A
        DEC   HL
        DEC   BC
        DEC   DE
        LD    A,(BC)
        LD    (HL),A
        DEC   HL
        DEC   BC
        DEC   DE
        LD    A,(BC)
        LD    (HL),A
        DEC   HL
        DEC   BC
        DEC   DE
        LD    A,(BC)
        LD    (HL),A
        DEC   HL
        DEC   BC
        DEC   DE
        JP    expandTailQuads
expandTailRemainder:  LD    A,E
        OR    A
        JP    Z,finishExpandTail
expandTailByte:  LD    A,(BC)
        LD    (HL),A
        DEC   HL
        DEC   BC
        DEC   E
        JP    NZ,expandTailByte
finishExpandTail:  POP   DE
        RET

keyHandler_Tab:
        CALL  testShiftKey
        JP    Z,tabPreviousStop
        LD    HL,0001H    ; 1
        CALL  addCursorColumn
        LD    E,L
        LD    HL,t_tabSteps             ; 53347
        LD    C,03H         ; 3
        XOR   A
tabNextInitialStop:  DEC   C
        JP    Z,tabNextRepeatedStop
        ADD   A,(HL)
        CP    E
        INC   HL
        JP    C,tabNextInitialStop
clampCursorColumn:  CP    3EH           ; 62 '>'; [2]
        JP    C,setCursorColumn
        LD    A,3EH         ; 62 '>'
setCursorColumn:  LD    L,A           ; [3]
        ADD   A,A
        ADD   A,L
        LD    (bios_vars.cursorY+1),A
        RET
tabNextRepeatedStop:  ADD   A,(HL)                ; [2]
        CP    E
        JP    C,tabNextRepeatedStop
        JP    clampCursorColumn
tabPreviousStop:  LD    HL,0000H
        CALL  addCursorColumn
        LD    E,L
        LD    HL,t_tabSteps             ; 53347
        LD    C,03H         ; 3
        XOR   A
tabPreviousInitialStop:  DEC   C
        JP    Z,tabPreviousRepeatedStop
        ADD   A,(HL)
        CP    E
        INC   HL
        JP    C,tabPreviousInitialStop
        DEC   HL
        SUB   (HL)
        JP    setCursorColumn
tabPreviousRepeatedStop:  ADD   A,(HL)                ; [2]
        CP    E
        JP    C,tabPreviousRepeatedStop
        SUB   (HL)
        JP    setCursorColumn

keyHandler_F2:
        CALL  commitLineEdit         ; [2]
        LD    HL,(v_textBeginLo)
        EX    DE,HL
        LD    HL,(v_topLine)
        LD    B,16H         ; 22
pageUpLineLoop:  CALL  compareHLDE
        JP    Z,pageUpRedraw
        CALL  previousLine
        PUSH  HL
        CALL  previousCurrentLine
        POP   HL
        DEC   B
        JP    NZ,pageUpLineLoop
pageUpRedraw:  LD    (v_topLine),HL
        CALL  redrawPreservingCursor
        RET

keyHandler_F3:
        CALL  commitLineEdit
        LD    HL,(v_topLine)
        LD    BC,1619H    ; 5657
pageDownLineLoop:  LD    E,L
        LD    D,H
        CALL  nextLine
        LD    A,(HL)
        INC   A
        JP    NZ,pageDownAdvance
        LD    L,E
        LD    H,D
        JP    pageDownRedraw
pageDownAdvance:  PUSH  HL
        CALL  nextCurrentLine
        CALL  Z,bios_printChar
        POP   HL
        DEC   B
        JP    NZ,pageDownLineLoop
pageDownRedraw:  LD    (v_topLine),HL
        CALL  redrawPreservingCursor
        RET

keyHandler_Esc_End:
        CALL  nextCurrentLine
        JP    NZ,keyHandler_Esc_End
        LD    HL,(v_currentLine)
        LD    (v_topLine),HL
        LD    A,08H         ; 8
        LD    (bios_vars.cursorY),A
        JP    keyHandler_F2

keyHandler_Esc_L:
        LD    HL,redrawSelection    ; 54885
        PUSH  HL
        LD    HL,(v_selectionBegin)
        LD    A,H
        OR    L
        JP    NZ,extendSelection
        CALL  setSelectionBegin
        JP    setSelectionEnd

extendSelection:  EX    DE,HL
        LD    HL,(v_currentLine)
        CALL  nextLine
        EX    DE,HL
        CALL  compareHLDE
        JP    C,setSelectionEnd
setSelectionBegin:  LD    HL,(v_currentLine)
        LD    (v_selectionBegin),HL
        RET

setSelectionEnd:  LD    HL,(v_currentLine)   ; [2]
        CALL  nextLine
        LD    (v_selectionEnd),HL
        RET

keyHandler_Esc_D:
        LD    HL,(v_selectionBegin)
        LD    A,L
        OR    H
        JP    Z,Beep4
        EX    DE,HL
        LD    HL,(v_currentLine)
findLineBeforeSelection:  CALL  testLineSelected
        JP    NC,deleteSelectionRange
        CALL  previousLine
        JP    NZ,findLineBeforeSelection
deleteSelectionRange:  LD    (v_currentLine),HL
        LD    HL,0000H
        LD    (v_selectionBegin),HL
        LD    HL,(v_selectionEnd)
deleteRangeAndReposition:  PUSH  DE
        CALL  subtractHLFromDE
        LD    B,D
        LD    C,E
        EX    DE,HL
        LD    HL,(v_topLine)
        CALL  compareHLDE
        JP    C,adjustCurrentAfterDelete
        ADD   HL,BC
        LD    (v_topLine),HL
adjustCurrentAfterDelete:  LD    HL,(v_currentLine)
        CALL  compareHLDE
        JP    C,collapseDeletedRange
        ADD   HL,BC
        LD    (v_currentLine),HL
collapseDeletedRange:  EX    DE,HL
        POP   DE
        CALL  resizeTextRange
        CALL  recountCurrentLine
        LD    A,(bios_vars.cursorY)
        SUB   08H           ; 8
        LD    B,A
        JP    Z,redrawAfterDelete
findTopAfterDelete:  CALL  previousLine
        CALL  Z,moveUpIfAtDocumentEnd
        LD    A,B
        SUB   0AH           ; 10
        LD    B,A
        JP    NZ,findTopAfterDelete
redrawAfterDelete:  LD    (v_topLine),HL
        JP    redrawPreservingCursor
moveUpIfAtDocumentEnd:  PUSH  HL
        CALL  nextCurrentLine
        POP   HL
        RET   NZ
        LD    C,19H         ; 25
        JP    bios_printChar

keyHandler_Esc_U:
        LD    HL,(v_selectionBegin)
        LD    DE,0000H
        CALL  compareHLDE
        RET   Z
        EX    DE,HL
        LD    (v_selectionBegin),HL
redrawSelection:  CALL  saveCursor
        LD    HL,(v_topLine)
        CALL  drawPage
        JP    restoreCursor
documentOutOfMemory:  LD    HL,txtOutOfMemory             ; 53351; [2]
        CALL  bios_printString
        CALL  Beep4
        CALL  bios_getch
        JP    scanDocument

keyHandler_Esc_O:
        LD    HL,txtTapePrompt             ; 53370
        CALL  bios_printString
        CALL  promptTapeName
        JP    NZ,synchronizeTopAndRedraw
        LD    HL,(v_textBeginLo)
        CALL  sumTextUntilEnd
        PUSH  BC
        PUSH  DE
        CALL  writeTapeLeader
        EX    (SP),HL
        EX    DE,HL
        LD    HL,startupBannerAndLineBuffer
        LD    A,0E6H
        LD    B,05H
writeTapeNameSync:  CALL  writeTapeByte
        DEC   B
        JP    NZ,writeTapeNameSync
writeTapeNameLoop:  LD    A,(HL)
        CALL  writeTapeByte
        OR    A
        JP    Z,writeTapePayload
        INC   HL
        JP    writeTapeNameLoop
writeTapePayload:  CALL  writeTapeGap
        POP   DE
        LD    A,(v_textBeginLo)
        cpl
        INC   A
        LD    L,A
        LD    A,(v_textBeginHi)
        cpl
        INC   A
        LD    H,A
        ADD   HL,DE
        LD    A,0E6H
        CALL  writeTapeByte
        LD    A,L
        cpl
        CALL  writeTapeByte
        LD    A,H
        cpl
        LD    HL,(v_textBeginLo)
        CALL  writeTapeRange
        POP   BC
        LD    A,C
        CALL  writeTapeByte
        LD    A,B
        CALL  writeTapeByte
        JP    synchronizeTopAndRedraw
writeTapeLeader:  LD    D,04H         ; 4
        XOR   A
writeLeaderBlock:  LD    E,40H         ; 64 '@'
        XOR   55H           ; 85 'U'
        CALL  writeTapeRepeat
        DEC   D
        JP    NZ,writeLeaderBlock
        RET
writeTapeGap:  CALL  writeTapeZeroBlock
writeTapeZeroBlock:  XOR   A
        LD    E,A
writeTapeRepeat:  CALL  writeTapeByte         ; [2]
        DEC   E
        JP    NZ,writeTapeRepeat
        RET
writeTapeRange:  CALL  writeTapeByte         ; [2]
        CALL  compareHLDE
        LD    A,(HL)
        INC   HL
        JP    NZ,writeTapeRange
        JP    writeTapeByte
writeTapeByte:  LD    C,A           ; [9]
        JP    bios_tapeWrite

keyHandler_Esc_I:
        LD    B,00H

readOrVerifyTape:  LD    A,B           ; [2]
        LD    (v_tapeMode),A
        LD    HL,0000H
        LD    (v_selectionBegin),HL
        LD    HL,txtTapePrompt             ; 53370
        CALL  bios_printString
        CALL  promptTapeName
        JP    NZ,synchronizeTopAndRedraw
        CALL  readTapeHeader
        PUSH  HL
readTapeTextLoop:  CALL  readTapeByte
        LD    B,A
        LD    A,(v_tapeMode)
        INC   A
        JP    NZ,storeTapeTextByte
        LD    A,B
        CP    (HL)
        JP    NZ,tapeReadMismatch
storeTapeTextByte:  LD    (HL),B
        INC   HL
        INC   B
        JP    NZ,readTapeTextLoop
        LD    A,08H
        CALL  readTapeWord
        POP   HL
        PUSH  BC
        CALL  sumTextUntilEnd
        EX    (SP),HL
        LD    D,B
        LD    E,C
        CALL  compareHLDE
        JP    NZ,tapeReadMismatch
        POP   HL
        LD    (v_textEnd),HL
        JP    keyHandler_Esc_Home

tapeReadMismatch:  POP   HL
        LD    HL,txtTapeError
        CALL  bios_printString
        CALL  Beep4
        CALL  bios_getch
        LD    HL,(v_textEnd)
        LD    (HL),0FFH
        JP    keyHandler_Esc_Home

readAndCompareTapeName:  LD    HL,smc_sessionTarget+1 ; тут адрес перехода в п/п restartAfterLoad
        CALL  findTapeNameSync
readTapeNameLoop:  CALL  readTapeByte
        LD    (HL),A
        OR    A
        JP    Z,showTapeName
        INC   HL
        JP    readTapeNameLoop

showTapeName:  LD    HL,txtTapeFile
        CALL  bios_printString
        LD    HL,smc_sessionTarget+1 ; тут адрес перехода в п/п restartAfterLoad
        PUSH  HL
        CALL  bios_printString
        CALL  tapeNamePause
        POP   HL
        LD    DE,startupBannerAndLineBuffer
compareTapeNameLoop:  LD    A,(DE)
        OR    A
        RET   Z
        CP    (HL)
        INC   HL
        INC   DE
        JP    Z,compareTapeNameLoop
        RET
tapeNamePause:  LD    HL,0000H
        LD    E,L
        LD    D,H
tapePauseLoop:  DEC   HL
        CALL  compareHLDE
        RET   Z
        JP    tapePauseLoop
readTapeWordSync:  LD    A,0FFH
readTapeWord:  CALL  bios_tapeRead
        LD    C,A
        CALL  readTapeByte
        LD    B,A
        RET
resetTapePolarity:  XOR   A
        LD    (bios_vars.tapeInverse),A

findTapeNameSync:  LD    B,04H
        LD    A,0FFH
findTapeSyncLoop:  CALL  bios_tapeRead
        CP    0E6H
        JP    NZ,resetTapePolarity
        DEC   B
        LD    A,08H
        JP    NZ,findTapeSyncLoop
        RET

readTapeByte:  LD    A,08H
        JP    bios_tapeRead

readTapeHeader:  CALL  readAndCompareTapeName
        JP    NZ,readTapeHeader
        CALL  readTapeWordSync
        LD    HL,(v_textBeginLo)
        LD    A,(v_tapeMode)
        DEC   A
        JP    M,checkTapeCapacity
        LD    HL,(v_textEnd)
checkTapeCapacity:  LD    A,B
        cpl
        LD    B,A
        LD    A,C
        cpl
        LD    C,A
        PUSH  HL
        EX    DE,HL
        LD    HL,(v_textLimitLo)
        EX    DE,HL
        ADD   HL,BC
        CALL  compareHLDE
        POP   HL
        RET   C
        JP    documentOutOfMemory

keyHandler_Esc_V:
        LD    B,0FFH                ; 255
        JP    readOrVerifyTape

keyHandler_Esc_G:
        LD    B,01H         ; 1
        JP    readOrVerifyTape

sumTextUntilEnd:  LD    BC,0000H    ; [2]
sumTextByte:  LD    A,(HL)
        CP    0FFH          ; 255
        RET   Z
        ADD   A,C
        LD    C,A
        LD    A,00H
        ADC   A,B
        LD    B,A
        INC   HL
        JP    sumTextByte
promptTapeName:  LD    B,30H         ; 48 '0'; [2]
        LD    HL,startupBannerAndLineBuffer             ; 53258
        LD    E,L
        LD    D,H
promptTextKey:  CALL  bios_getch          ; [5]
promptTextInput:  CP    20H           ; 32 ' '
        JP    C,promptControlKey
        DEC   B
        JP    NZ,storePromptCharacter
        INC   B
        JP    promptTextKey
storePromptCharacter:  LD    C,A
        CALL  bios_printChar
        LD    (HL),A
        INC   HL
        JP    promptTextKey
        LD    BC,2A20H    ; 10784
        CALL  keyHandler_Home
        JP    TypeCharCtimesB
promptControlKey:  CP    1FH           ; 31
        JP    NZ,promptBackspace
        OR    A
        RET
promptBackspace:  CP    08H           ; 8
        JP    NZ,promptEnter
        CALL  compareHLDE
        JP    Z,promptTextKey
        PUSH  HL
        LD    HL,txtEraseChar    ; 53380
        CALL  bios_printString
        POP   HL
        DEC   HL
        INC   B
        JP    promptTextKey
promptEnter:  CP    0DH           ; 13
        JP    NZ,promptTextKey
        XOR   A
        LD    (HL),A
        RET

keyHandler_F7:
        CALL  commitLineEdit
        LD    HL,06F9H    ; 1785
        LD    (bios_vars.cursorY),HL
        CALL  enableInverse
        LD    HL,txtSearchPrompt    ; 53395
        CALL  bios_printString
        LD    HL,1EF9H    ; 7929
        LD    (bios_vars.cursorY),HL
        CALL  bios_getch
        CP    0DH           ; 13
        JP    Z,repeatSearch
        LD    BC,1020H    ; 4128
        CALL  TypeCharCtimesB
        LD    (bios_vars.cursorY),HL
        LD    HL,v_searchText    ; 53403
        LD    E,L
        LD    D,H
        LD    B,0FH         ; 15
        CALL  promptTextInput
        PUSH  AF
        CALL  disableInverse
        POP   AF
        JP    NZ,synchronizeTopAndRedraw
repeatSearch:  CALL  disableInverse
        LD    HL,(v_topLine)
        CALL  nextLine
searchCandidate:  LD    DE,v_searchText    ; 53403
compareSearchByte:  LD    B,(HL)
        LD    A,(DE)
        OR    A
        JP    Z,searchFound
        CP    B
        INC   HL
        INC   DE
        JP    Z,compareSearchByte
        INC   B
        JP    NZ,searchCandidate
        CALL  Beep4
        JP    keyHandler_Esc_Home
searchFound:  CALL  previousLine
        LD    (v_topLine),HL
        LD    (v_currentLine),HL
        CALL  recountCurrentLine
        JP    synchronizeTopAndRedraw

keyHandler_Esc_J:
        CALL  beginLineEdit
        CALL  trimLineBuffer
        LD    HL,(v_currentLine)
        CALL  nextLine
        LD    A,(HL)
        INC   A
        RET   Z
        DEC   HL
        LD    (HL),20H    ; 32 ' '
        INC   HL
        LD    C,L
        LD    B,H
        LD    HL,lineJoinLimit             ; 53319
joinLineByte:  CALL  compareHLDE
        JP    C,commitAndRedraw
        LD    A,(BC)
        CP    0DH           ; 13
        JP    Z,commitAndRedraw
        CP    0FFH          ; 255
        JP    Z,commitAndRedraw
        LD    (DE),A
        INC   DE
        INC   BC
        JP    joinLineByte

keyHandler_Esc_S:
        LD    HL,(v_currentLine)
        CALL  addCursorColumn
        LD    E,L
        LD    D,H
        INC   DE
        PUSH  HL
        CALL  resizeTextRange
        POP   HL
        LD    (HL),0DH    ; 13
        JP    redrawPreservingCursor

previousLine:  PUSH  DE            ; [6]
        EX    DE,HL
        LD    HL,(v_textBeginLo)
        EX    DE,HL
        CALL  compareHLDE
        JP    Z,previousLineReturn
        DEC   HL
        CALL  compareHLDE
        JP    Z,previousLineFound
        DEC   HL
findPreviousLineLoop:  LD    A,(HL)
        CP    0DH           ; 13
        JP    Z,previousLineAfterCR
        CALL  compareHLDE
        DEC   HL
        JP    NZ,findPreviousLineLoop

previousLineAfterCR:  INC   HL
previousLineFound: XOR   A
        INC   A

previousLineReturn: POP   DE
        RET

keyHandler_Esc_C:
        LD    HL,(v_selectionEnd)
        EX    DE,HL
        LD    HL,(v_selectionBegin)
        LD    A,H
        OR    L
        JP    Z,Beep4
        CALL  subtractHLFromDE
        LD    HL,(v_currentLine)
        CALL  testLineSelected
        PUSH  AF
        CALL  nextLine
        POP   AF
        CALL  C,testLineSelected
        JP    C,Beep4
        PUSH  HL
        EX    DE,HL
        ADD   HL,DE
        EX    DE,HL
        CALL  resizeTextRange
        LD    HL,(v_selectionBegin)
        LD    C,L
        LD    B,H
        POP   HL
        CALL  copyUntilDE
        JP    redrawPreservingCursor

keyHandler_Esc_M:
        LD    HL,(v_selectionEnd)
        EX    DE,HL
        LD    HL,(v_selectionBegin)
        LD    A,H
        OR    L
        JP    Z,Beep4
        CALL  subtractHLFromDE
        LD    HL,(v_currentLine)
        CALL  testLineSelected
        JP    C,Beep4
        CALL  nextLine
        PUSH  HL
        EX    DE,HL
        ADD   HL,DE
        EX    DE,HL
        POP   BC
        PUSH  DE
        PUSH  HL
        PUSH  BC
        CALL  resizeTextRange
        LD    HL,(v_selectionBegin)
        LD    C,L
        LD    B,H
        POP   HL
        CALL  copyUntilDE
        LD    HL,(v_selectionBegin)
        EX    DE,HL
        POP   HL
        LD    (v_selectionBegin),HL
        LD    HL,(v_selectionEnd)
        EX    (SP),HL
        LD    (v_selectionEnd),HL
        POP   HL
        JP    deleteRangeAndReposition

keyHandler_Esc_N:
        LD    HL,txtNew    ; 53419
        CALL  bios_printString
        CALL  bios_getch
        CP    59H           ; 89 'Y'
        JP    NZ,synchronizeTopAndRedraw
        LD    HL,(v_textBeginLo)
        LD    (HL),0DH    ; 13
        INC   HL
        LD    (HL),0FFH   ; 255
        LD    (v_textEnd),HL
        JP    keyHandler_Esc_Home

keyHandler_F8:
        XOR   A
        LD    (v_lineDirty),A
        CALL  saveCursor
        CALL  keyHandler_Home
        LD    BC,3F20H    ; 16160
        CALL  TypeCharCtimesB
        LD    HL,(v_currentLine)
        CALL  drawTextLine
        JP    restoreCursor
testShiftKey:  LD    A,(IO_KEYB_B)  ; [3]
        AND   02H           ; 2
        RET
copyUntilDE:  LD    A,(BC)                ; [5]
        LD    (HL),A
        INC   HL
        INC   BC
        CALL  compareHLDE
        JP    NZ,copyUntilDE
        RET
compareHLDE:  LD    A,H           ; [34]
        CP    D
        RET   NZ
        LD    A,L
        CP    E
        RET
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

nextLine:  LD    A,(HL)                ; [11]
        CP    0FFH          ; 255
        RET   Z
        CP    0DH           ; 13
        INC   HL
        JP    NZ,nextLine
        RET
saveCursor:  PUSH  HL            ; [7]
        LD    HL,(bios_vars.cursorY)
        LD    (smc_restoreCursor+1),HL
        POP   HL
        RET
restoreCursor:  PUSH  HL            ; [7]
smc_restoreCursor:   LD    HL,0000H
        LD    (bios_vars.cursorY),HL
        POP   HL
        RET
resumeAtTextPointer: CALL  resetSessionStack
        CALL  previousLine
        LD    (v_currentLine),HL
        LD    (v_topLine),HL
        CALL  recountCurrentLine
        CALL  drawStatusBar
        CALL  drawPage
        CALL  bios_getch
        PUSH  AF
        LD    HL,06F9H    ; 1785
        LD    (bios_vars.cursorY),HL
        LD    BC,3020H    ; 12320
        CALL  TypeCharCtimesB
        CALL  finishPageAndHome
        POP   AF
        LD    HL,editorStatusLoop    ; 53535
        PUSH  HL
        JP    keyInput2
scrollTextUp:  LD    H,90H         ; 144
        LD    D,H
scrollUpColumn: LD    C,39H         ; 57 '9'
        LD    L,01H         ; 1
        LD    E,0BH         ; 11
scrollUpFourBytes: LD    A,(DE)
        LD    (HL),A
        INC   HL
        INC   DE
        LD    A,(DE)
        LD    (HL),A
        INC   HL
        INC   DE
        LD    A,(DE)
        LD    (HL),A
        INC   HL
        INC   DE
        LD    A,(DE)
        LD    (HL),A
        INC   HL
        INC   DE
        DEC   C
        JP    NZ,scrollUpFourBytes
        LD    C,0AH         ; 10
        XOR   A
clearScrollUpBottom: LD    (HL),A
        INC   HL
        DEC   C
        JP    NZ,clearScrollUpBottom
        LD    A,H
        CP    0BFH          ; 191
        RET   Z
        INC   H
        INC   D
        JP    scrollUpColumn
insertScreenRow:  LD    A,(bios_vars.cursorY)  ; [2]
        LD    L,A
        LD    A,0EEH                ; 238
        SUB   L
        OR    A
        rra
        LD    (smc_scrollPairCount+1),A
        LD    H,0BFH                ; 191
        LD    D,H
        LD    B,30H         ; 48 '0'
scrollDownColumn: LD    L,0EFH                ; 239
        LD    E,0E5H                ; 229
smc_scrollPairCount:   LD    A,00H
        LD    C,A
        OR    A
        JP    Z,clearInsertedRow
scrollDownPair: LD    A,(DE)
        LD    (HL),A
        DEC   HL
        DEC   DE
        LD    A,(DE)
        LD    (HL),A
        DEC   HL
        DEC   DE
        DEC   C
        JP    NZ,scrollDownPair
clearInsertedRow: LD    C,09H         ; 9
        XOR   A
clearInsertedRowByte: LD    (HL),A
        DEC   HL
        DEC   DE
        DEC   C
        JP    NZ,clearInsertedRowByte
        DEC   H
        LD    D,H
        DEC   B
        JP    NZ,scrollDownColumn
        RET
updateEditMode:  PUSH  HL            ; [2]
        LD    HL,(bios_vars.cursorY)
        PUSH  HL
        CALL  enableInverse
        LD    HL,9DF9H    ; 40441
        LD    (bios_vars.cursorY),HL
        LD    A,(v_insertMode)
        OR    A
        LD    HL,txtIns    ; 53428
        JP    NZ,printEditMode
        LD    HL,txtOvr    ; 53438
printEditMode: CALL  bios_printString
        CALL  disableInverse
        POP   HL
        LD    (bios_vars.cursorY),HL
        POP   HL
        RET
nextCurrentLine:  LD    HL,(v_currentLine)   ; [5]
        CALL  nextLine
        LD    A,(HL)
        INC   A
        RET   Z
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
updateLineNumber:  PUSH  DE            ; [3]
        PUSH  BC
        PUSH  HL
        LD    HL,(bios_vars.cursorY)
        PUSH  HL
        CALL  enableInverse
        LD    HL,67F9H    ; 26617
        LD    (bios_vars.cursorY),HL
        LD    HL,(v_lineNumber)
        LD    B,20H         ; 32 ' '
        LD    DE,0FC18H   ; 64536
        CALL  printDecimalDigit
        LD    DE,03E8H    ; 1000
        ADD   HL,DE
        LD    DE,0FF9CH   ; 65436
        CALL  printDecimalDigit
        LD    DE,0064H    ; 100
        ADD   HL,DE
        LD    DE,0FFF6H   ; 65526
        CALL  printDecimalDigit
        LD    A,L
        ADD   A,3AH         ; 58 ':'
        LD    C,A
        CALL  bios_printChar
        CALL  disableInverse
        POP   HL
        LD    (bios_vars.cursorY),HL
        POP   HL
        POP   BC
        POP   DE
        RET
printDecimalDigit:  LD    C,2FH         ; 47 '/'; [4]
decimalDigitLoop: INC   C
        ADD   HL,DE
        JP    C,decimalDigitLoop
        LD    A,C
        CP    30H           ; 48 '0'
        JP    Z,decimalDigitPrint
        LD    B,0FFH                ; 255
decimalDigitPrint: AND   B
        LD    C,A
        JP    bios_printChar
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
drawStatusBar:  PUSH  HL            ; [2]
        LD    HL,(bios_vars.cursorY)
        PUSH  HL
        CALL  enableInverse
        LD    H,90H         ; 144
        LD    C,30H         ; 48 '0'
statusBarColumn: LD    L,0F1H                ; 241
        LD    A,09H         ; 9
statusBarPixel: LD    (HL),0FFH   ; 255
        INC   L
        DEC   A
        JP    NZ,statusBarPixel
        INC   H
        DEC   C
        JP    NZ,statusBarColumn
        LD    HL,58F9H    ; 22777
        LD    (bios_vars.cursorY),HL
        LD    HL,txtLine    ; 53448
        CALL  bios_printString
        LD    HL,80F9H    ; 33017
        LD    (bios_vars.cursorY),HL
        LD    HL,txtCol    ; 53453
        CALL  bios_printString
        CALL  updateLineNumber
        CALL  updateEditMode
        CALL  disableInverse
        POP   HL
        LD    (bios_vars.cursorY),HL
        POP   HL
        RET

resetSessionStack:  POP   HL

        ; Установка указателя стека, агрумент этой инструкции
        ; может меняться из п/п beginSession
smc_resetSessionStack:   LD    SP,8F50H
        ; Переход по адресу в hl
        JP    (HL)

moveNextWord: CALL  beginLineEdit
        CALL  getLineBufferCursor
        LD    DE,lineWordRightLimit             ; 53320
        LD    C,18H         ; 24
skipCurrentWordRight: CALL  compareHLDE
        RET   Z
        LD    A,(HL)
        CP    20H           ; 32 ' '
        JP    Z,skipSpacesRight
        INC   HL
        CALL  bios_printChar
        JP    skipCurrentWordRight
skipSpacesRight: CALL  compareHLDE         ; [2]
        JP    Z,keyHandler_End
        LD    A,(HL)
        CP    20H           ; 32 ' '
        RET   NZ
        INC   HL
        CALL  bios_printChar
        JP    skipSpacesRight
movePreviousWord: CALL  beginLineEdit
        CALL  getLineBufferCursor
        LD    DE,startupBannerAndLineBuffer             ; 53258
        LD    C,08H         ; 8
        CALL  compareHLDE
        RET   Z
        DEC   HL
        DEC   DE
skipSpacesLeft: CALL  compareHLDE
        RET   Z
        LD    A,(HL)
        CP    20H           ; 32 ' '
        JP    NZ,skipCurrentWordLeft
        CALL  bios_printChar
        DEC   HL
        JP    skipSpacesLeft
skipCurrentWordLeft: CALL  compareHLDE         ; [2]
        RET   Z
        LD    A,(HL)
        CP    20H           ; 32 ' '
        RET   Z
        DEC   HL
        CALL  bios_printChar
        JP    skipCurrentWordLeft
clearTextViewport:  PUSH  HL
        PUSH  DE
        LD    HL,0000H
        ADD   HL,SP
        LD    (smc_restoreClearStack+1),HL
        LD    DE,0000H
        LD    HL,0BFF0H   ; 49136
        LD    C,30H         ; 48 '0'
clearTextColumn: LD    SP,HL
        LD    A,0FH         ; 15
clearTextPushLoop: PUSH  DE
        PUSH  DE
        PUSH  DE
        PUSH  DE
        PUSH  DE
        PUSH  DE
        PUSH  DE
        PUSH  DE
        DEC   A
        JP    NZ,clearTextPushLoop
        DEC   H
        DEC   C
        JP    NZ,clearTextColumn
smc_restoreClearStack:   LD    SP,0FFFFH   ; 65535
        POP   DE
        POP   HL
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

inputFileName:
        CALL  prepareFilePrompt
        LD    DE,FILE_NAME_RECALL_END
        LD    BC,inputFileNameBuffer    ; 56771
        CALL  bios_memcpy_bc_hl
        LD    HL,00F9H    ; 249
        LD    (bios_vars.cursorY),HL
        LD    HL,0FFFFH   ; 65535
        LD    (bios_vars.inverse),HL
        LD    HL,txtFile    ; 56766
        CALL  bios_printString
        CALL  bios_getch
        LD    HL,0FF9H    ; 4089
        LD    (bios_vars.cursorY),HL
        CP    0DH           ; 13
        JP    Z,finishFilePrompt
        PUSH  AF
        LD    HL,txtSpace12    ; 56800
        CALL  bios_printString
        LD    HL,0FF9H    ; 4089
        LD    (bios_vars.cursorY),HL
        LD    HL,inputFileNameBuffer    ; 56771
        LD    BC,inputFileNameBuffer    ; 56771
        LD    DE,inputBufEnd    ; 56781
        POP   AF
        JP    fileNameInputKey

finishFilePrompt: LD    HL,inputFileNameBuffer
        LD    DE,inputBufEnd
        LD    BC,FILE_NAME_HISTORY
        CALL  bios_memcpy_bc_hl
        LD    HL,0000H
        LD    (bios_vars.inverse),HL
        RET

;----------------------------------------------------------------------
; Загрузка файла с диска

openFile:
        CALL  prepareFileName
        LD    HL,(v_textBeginLo)
        EX    DE,HL
        LD    HL,fileDescName
        CALL  bios_fileLoad2
        JP    C,fileOperationError
        JP    finishFilePrompt

;----------------------------------------------------------------------
; Подготовка имени файла для файловых операций

prepareFileName:
        LD    HL,inputFileNameBuffer
        LD    DE,fileDescName
        CALL  bios_fileNamePrepare
        RET

;----------------------------------------------------------------------
; Сохранение файла на диск

saveFile:
        CALL  prepareFileName
        LD    DE,0000H
        LD    B,00H
        LD    HL,(v_textBeginLo)

        ; Подсчет контрольной суммы
getCRC: LD    A,B
        ADD   A,(HL)
        LD    B,A
        INC   DE
        LD    A,(HL)
        INC   A
        JP    Z,buildSaveDescriptor
        INC   HL
        JP    getCRC

        ; Формирование дескриптора файла
buildSaveDescriptor: LD    HL,(v_textBeginLo)
        LD    (fileDescAddr),HL
        EX    DE,HL
        LD    (fileDescSize),HL
        LD    A,B
        LD    (fileDescCRC),A

        ; Сохранение файла
        LD    HL,fileDescName
        CALL  bios_fileCreate
        JP    C,fileOperationError
        JP    finishFilePrompt

;----------------------------------------------------------------------

fileNameOverflow: POP   AF
        JP    synchronizeTopAndRedraw
        LD    B,H
        LD    C,L
fileNameInputLoop: CALL  bios_getch          ; [4]
fileNameInputKey: CP    08H           ; 8
        JP    Z,fileNameBackspace
        CP    0DH           ; 13
        JP    Z,terminateFileName
        CP    1FH           ; 31
        JP    Z,cancelFilePrompt
        CP    20H           ; 32 ' '
        JP    C,fileNameInputLoop
        PUSH  AF
        CALL  bios_cmp_hl_de
        JP    Z,fileNameOverflow
        POP   AF
        LD    (HL),A
        PUSH  BC
        LD    C,A
        CALL  bios_printChar
        POP   BC
        INC   HL
        JP    fileNameInputLoop
fileNameBackspace: LD    A,H
        CP    B
        JP    NZ,eraseFileNameCharacter
        LD    A,L
        CP    C
        JP    Z,fileNameInputLoop
eraseFileNameCharacter: DEC   HL
        PUSH  BC
        LD    C,08H         ; 8
        CALL  bios_printChar
        LD    C,20H         ; 32 ' '
        CALL  bios_printChar
        LD    C,08H         ; 8
        CALL  bios_printChar
        POP   BC
        JP    fileNameInputLoop
terminateFileName: LD    (HL),00H
        JP    finishFilePrompt
fileOperationError:  LD    C,05H         ; 5; [5]
fileErrorBeepLoop: CALL  bios_beep_Old
        LD    B,0FFH                ; 255
        CALL  bios_delay_b
        CALL  bios_delay_b
        DEC   C
        JP    NZ,fileErrorBeepLoop
        JP    finishFilePrompt
;----------------------------------------------------------------------

insertFile:
        CALL  prepareFileName
        LD    HL,fileDescName

        ; Запрашиваем дескриптор файла
        CALL  bios_fileLoadInfo
        LD    HL,(fileDescSize)
        EX    DE,HL         ; de = размер файла
        PUSH  DE
        LD    HL,(v_textBeginLo)    ; hl = адрес буфера

        ; Сканируем буфер, пока не найдем конец текста (0FFh)
findAppendPosition: LD    A,(HL)
        INC   A
        JP    Z,checkAppendCapacity
        INC   HL
        JP    findAppendPosition

checkAppendCapacity: POP   DE    ; de = размер файла
        PUSH  HL    ; hl = адрес конца буфера
        ADD   HL,DE
        EX    DE,HL
        LD    HL,(v_textLimitLo)
        LD    A,H
        CP    D
        JP    C,fileOperationError
        JP    NZ,loadAppendFile
        LD    A,L
        CP    E
        JP    C,fileOperationError

        ; загрузка файла по адресу de
loadAppendFile: POP   DE
        LD    HL,fileDescName
        CALL  bios_fileLoad2
        JP    C,fileOperationError
        JP    finishFilePrompt

;----------------------------------------------------------------------

cancelFilePrompt: POP   DE
        LD    HL,0000H
        LD    (bios_vars.inverse),HL
        JP    clearAndRedrawDocument

;----------------------------------------------------------------------

txtFile:
        DB    "FILE:"

inputFileNameBuffer:
        BLOCK 10, 00h

inputBufEnd:  DB    00H,00H,00H         ; "..."

; Дескриптор файла (запись в каталоге MXOS)
fileDescName:   BLOCK 10, 20h   ; имя, расширение и атрибуты
fileDescAddr:   DW    2020H     ; адрес загрузки
fileDescSize:   DW    2020H     ; размер
fileDescCRC:    DB    20H       ; контрольная сумма
                DB    20H       ; первый сектор

txtSpace12:  DB    20H,20H,20H,20H     ; "    "
        DB    20H,20H,20H,20H     ; "    "
        DB    20H,20H,20H,20H     ; "    "
        DB    00H                 ; "."

;----------------------------------------------------------------------

        ; Обработка кода нажатой клавиши
keyInput:
        CALL  bios_getch
        LD    C,A
        CP    03H               ; F4 - открыть файл
        JP    Z,keyHandler_F4
        CP    04H               ; F5 - сохранить файл на диск
        JP    Z,keyHandler_F5
        CP    05H               ; F6 - открыть файл и вставить его в конец документа
        JP    Z,keyHandler_F6
        JP    keyInput2

;----------------------------------------------------------------------

keyHandler_F4:
        CALL  inputFileName
        CALL  openFile
        JP    restartAfterLoad

keyHandler_F5:
        CALL  inputFileName
        CALL  saveFile
        JP    synchronizeTopAndRedraw

keyHandler_F6:
        CALL  inputFileName
        CALL  insertFile
        JP    restartAfterLoad

;----------------------------------------------------------------------

        ; ???
        NOP

prepareFilePrompt:
        CALL  commitLineEdit
        LD    HL,FILE_NAME_HISTORY
        RET
openCommandLineFile:
        EX    DE,HL
        LD    DE,commandLineFileNameBuffer
        CALL  bios_fileNamePrepare
        LD    HL,(v_textBeginLo)
        EX    DE,HL
        LD    HL,commandLineFileNameBuffer
        CALL  bios_fileLoad2
        JP    startupBannerAndLineBuffer

        DB    00H,00H
commandLineFileNameBuffer:  DB    00H,00H,00H,00H
