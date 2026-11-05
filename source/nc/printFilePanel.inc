;----------------------------------------------------------------------------
; MXOS NC.COM
; Вывод списка файлов на панель
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

;----------------------------------------------------------------------------
; Перерисовка всей панели

redrawFilePanel:
        ld a,(ncSelectiveRefresh)
        or a
        call z,invalidateFileCache
        COLOR   COLOR_BORDER
        ld a,(ncKeepFrames)
        or a
        jp nz,refreshFilePanel
        call    drawPanelTop

        ; Рисование панели
        ld      hl, g_filePanel
        call    draw

        ; Выодим список файлов сверху вниз
        ld      b, 0
        jp printFilePanel

; После тихой команды обновляем только содержимое и заголовок панели.
refreshFilePanel:
        call drawPanelHeader
        ld hl,0910Ah
        call activePanelPos
        call clearFileColumn
        ld hl,09D0Ah
        call activePanelPos
        call clearFileColumn
        ld b,0
        jp printFilePanel
clearFileColumn:
        ld b,10
clearFileColumnByte:
        ld l,10
        ld c,198
        xor a
clearFileColumnRow:
        ld (hl),a
        inc l
        dec c
        jp nz,clearFileColumnRow
        inc h
        dec b
        jp nz,clearFileColumnByte
        ret

;----------------------------------------------------------------------------
; Перерисовка списка файлов
; вход:
;   b - режим (0 - вся панель, 1 - прокрутка снизу вверх, 2 - сверху вниз)

printFilePanel:
        ; Рисуем сверху вниз или снизу вверх?
        ld      a, b
        or      a
        push    af

        ; Загрузка списка файлов
        ld      hl, state.panelA_drive
        call    readBytePanel   ; a = номер накопителя
        call    loadFiles

        pop af
        push af
        call z,printPanelPath

        ; Цвет для печати списка файлов
        COLOR   COLOR_FILE

        ld      b, FILE_LIST_SIZE
        pop     af
        jp z,   printFilesUpDown
        dec     a
        jp z,   printFilesDownUp

printFilesUpDownScroll:
        ; При прокрутке назад выбран верхний левый файл.
        ld      hl, (vars.fileListPtr)
        ld      de, (P_FILE_LIST_X1 << 8) + P_FILE_LIST_Y
        call    printFirstScrollFile
        jp      pfud_Next

;----------------------------------------------------------------------------
; Цикл печати сверху вниз

printFilesUpDown:
        ; Указатель списка файлов - на нулевой файл
        ld      hl, (vars.fileListPtr)

        ; Положение курсора - на нулевой файл
        ld      de, (P_FILE_LIST_X1 << 8) + P_FILE_LIST_Y

pfud_Loop:
        ; Прерываем цикл по достижении конца списка файлов
        ; (первый символ имени = FF)
        ld      a, (hl)
        inc     a
        ret z

        call    printPanelFile

pfud_Next:
        dec     b
        ret z
        ; Следующий файл
        push    bc
        ld      bc, FILE_INFO_SIZE
        add     hl, bc
        pop     bc

        ; Курсор на следующую строку
        ld      a, e
        add     10
        ld      e, a

        ; Курсор на следующий столбец
        cp      P_FILE_LIST_Y_MAX
        jp c,   pfud_Loop
        ld      de, (P_FILE_LIST_X2 << 8) + P_FILE_LIST_Y
        jp      pfud_Loop

;----------------------------------------------------------------------------
; Цикл печати снизу вверх

printFilesDownUp:
        ; Указатель списка файлов - на последний файл
        ld      hl, (vars.fileListPtr)
        ld      de, (FILE_LIST_SIZE-1) * FILE_INFO_SIZE
        add     hl, de

        ; Положение курсора - на последний файл
        ld      de, (P_FILE_LIST_X2 << 8) + P_FILE_LIST_Y_MAX - 10

        ; При прокрутке вперёд выбран нижний правый файл.
        call    printFirstScrollFile
        jp      pfdu_Next

pfdu_Loop:
        call    printPanelFile

pfdu_Next:
        dec     b
        ret z
        ; Предыдущий файл
        push    bc
        ld      bc, -FILE_INFO_SIZE
        add     hl, bc
        pop     bc

        ; Курсор на предыдущую строку
        ld      a, e
        sub     10
        ld      e, a

        ; Курсор на предыдущий столбец
        cp      P_FILE_LIST_Y
        jp nc,  pfdu_Loop
        ld      de, (P_FILE_LIST_X1 << 8) + P_FILE_LIST_Y_MAX - 10
        jp      pfdu_Loop

;----------------------------------------------------------------------------
; Первая строка прокрутки: имя, затем курсор до остальных строк.
; Вход: HL = файл, DE = координаты. Сохраняет HL/DE/BC, разрушает AF.
; Курсор уже нарисован; вызывающий код пропускает его в mainLoop.

printFirstScrollFile:
    IF NC_BITMAP_SCROLL
        ld      a, (vars.scrollCursorKept)
        or      a
        jp nz,  printKeptScrollFile
    ENDIF
        call    printPanelFile
        push    hl
        push    de
        push    bc
        COLOR   COLOR_CURSOR
        call    drawCursor
        COLOR   COLOR_FILE
        pop     bc
        pop     de
        pop     hl
        ret

; Вход: HL = файл, DE = координаты. Сохраняет HL/DE/BC, разрушает AF.
printPanelFile:
        ex      de, hl
        call    setCursorPosPanel
        ex      de, hl
        jp      printFileNamOnPanel

    IF NC_BITMAP_SCROLL
; Старая инверсия остаётся на краю окна до печати нового имени.
; Не скопированный соседний элемент восстанавливаем обычным текстом.
printKeptScrollFile:
        push    hl
        push    de
        push    bc
        ld      a, e
        ld      bc, FILE_INFO_SIZE
        cp      P_FILE_LIST_Y
        jp z,   pks_Below
        ld      bc, -FILE_INFO_SIZE
        sub     10
        jp      pks_Normal
pks_Below:
        add     10
pks_Normal:
        ld      e, a
        add     hl, bc
        call    printPanelFile
        pop     bc
        pop     de
        pop     hl
        COLOR   COLOR_CURSOR
        ld      a, 1
        ld      (bios_vars.inverse), a
        call    printPanelFile
        xor     a
        ld      (bios_vars.inverse), a
        COLOR   COLOR_FILE
        ret
    ENDIF
