;----------------------------------------------------------------------------
; MXOS NC.COM
; Вывод списка файлов на панель
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

;----------------------------------------------------------------------------
; Перерисовка всей панели

redrawFilePanel:
        ld a,(ncKeepFrames)
        or a
        jp nz,refreshFilePanel
        call ncResetPanelBorder
        ; Полная отрисовка уже очищает содержимое окна.
        xor a
        ld hl,vars.drawnLeftA
        call writeBytePanel
        ld hl,vars.drawnRightA
        call writeBytePanel
        call    drawPanelTop

        ; Рисование панели
        ld      hl, g_filePanel
        call    draw
        call ncDrawFileDividers

        ; Выодим список файлов сверху вниз
        ld      b, 0
        jp printFilePanel

; После тихой команды обновляем только содержимое и заголовок панели.
refreshFilePanel:
        call ncSetPanelBorder
        call drawPanelHeader
        ld b,0
        jp printFilePanel

;----------------------------------------------------------------------------
; Перерисовка списка файлов
; вход:
;   b - режим (0 - вся панель, 1 - прокрутка снизу вверх, 2 - сверху вниз)

printFilePanel:
        ld a,(ncFontWidth)
        cp 6
        jp z,ncLegacyPanel
        push bc
        call ncPreparePrintFilePanel
        COLOR COLOR_FILE
        pop bc
        ld a,b
        or a
        jp z,printWholeFileList
        call ncPrintAdaptiveList
        COLOR COLOR_CURSOR
        jp drawCursor
ncLegacyPanel:
        ; Рисуем сверху вниз или снизу вверх?
        push    bc
        call    ncPreparePrintFilePanel
        pop     bc

        ; Цвет для печати списка файлов
        COLOR   COLOR_FILE

        ld      a,b
        or      a
        jp z,   printWholeFileList
        ld      b, FILE_LIST_SIZE
        dec     a
        jp z,   printFilesDownUp

printFilesUpDownScroll:
        ; При прокрутке назад выбран верхний левый файл.
        ld      hl, (vars.fileListPtr)
        ld      de, (P_FILE_LIST_X1 << 8) + P_FILE_LIST_Y
        call    printFirstScrollFile
        jp      pfud_Next

ncPreparePrintFilePanel:
        ld a,b
        or a
        jp nz,lf_UpdateViewport
        ld hl,state.panelA_drive
        call readBytePanel
        call loadFiles
        jp printPanelPath

;----------------------------------------------------------------------------
; Цикл печати сверху вниз

printWholeFileList:
        ld a,(ncFontWidth)
        cp 6
        jp nz,pfAdaptiveWhole
        ; Полная отрисовка вызывается и напрямую после стадий рамок/метаданных.
        ; loadFiles разрушает B: число видимых строк задаём здесь, не у caller.
        ld b,FILE_LIST_SIZE
        call printFilesUpDown
        call clearPanelTail
        jp pfShowDiskStatus
pfAdaptiveWhole:
        call ncPrintAdaptiveList
pfShowDiskStatus:
        ld hl,vars.panelA_diskError
        call readBytePanel
        cp DOS_ERR_NO_MEDIA
        ret nz
        ld a,(ncFontWidth)
        cp 6
        jp nz,pfNoMediaText
        ; При следующей перерисовке очистить эту строку вместе со списком.
        ld a,1
        ld hl,vars.drawnLeftA
        call writeBytePanel
pfNoMediaText:
        ld hl,(P_FILE_LIST_X1 << 8)+P_FILE_LIST_Y
        call setCursorPosPanel
        ld hl,aNoMedia
        jp ncPrintPanelText

; Стираем только строки, оставшиеся после прежнего видимого списка.
; По две высоты принадлежат каждой панели; рамки и соседняя панель сохранены.
clearPanelTail:
        ld hl,state.panelA_firstFile
        call readBytePanel
        ld c,a
        ld hl,state.panelA_filesCnt
        call readBytePanel
        sub c
        jp nc,panelRowsNonnegative
        xor a
panelRowsNonnegative:
        cp FILE_LIST_SIZE
        jp c,panelRowsReady
        ld a,FILE_LIST_SIZE
panelRowsReady:
        ld (vars.newRows),a
        cp P_FILE_LIST_H
        jp c,panelLeftReady
        ld a,P_FILE_LIST_H
panelLeftReady:
        ld b,a
        ld hl,vars.drawnLeftA
        call readBytePanel
        ld c,a
        ld a,b
        call writeBytePanel
        ld hl,(SCROLL_LEFT << 8)+SCROLL_TOP
        call activePanelPos
        call clearColumnTail
        ld a,(vars.newRows)
        sub P_FILE_LIST_H
        jp nc,panelRightReady
        xor a
panelRightReady:
        ld b,a
        ld hl,vars.drawnRightA
        call readBytePanel
        ld c,a
        ld a,b
        call writeBytePanel
        ld hl,(SCROLL_RIGHT << 8)+SCROLL_TOP
        call activePanelPos
        jp clearColumnTail

; HL=верх колонки, B=новая высота, C=прежняя, в текстовых строках.
; Удаляем полезную ширину колонки, включая поля выделения слева и справа.
clearColumnTail:
        ld a,c
        sub b
        ret c
        ret z
        ld e,a
        ld a,b
        call panelRowsToPixels
        add a,l
        ld l,a
        ld a,e
        call panelRowsToPixels
        ld c,a
        ld b,10
    IF NC_BANKED
        xor a
        jp uiFillRectBytes
    ELSE
clearTailByte:
        push hl
        push bc
        xor a
clearTailPixel:
        ld (hl),a
        inc l
        dec c
        jp nz,clearTailPixel
        pop bc
        pop hl
        inc h
        dec b
        jp nz,clearTailByte
        ret
    ENDIF
panelRowsToPixels:
        add a,a
        ld d,a
        add a,a
        add a,a
        add a,d
        ret

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
        ld      bc, 1
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
        ld      de, FILE_LIST_SIZE-1
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
        ld      bc, -1
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
; Вход: HL = ячейка индекса, DE = координаты. Сохраняет HL/DE/BC, разрушает AF.
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

; Вход: HL = ячейка индекса, DE = координаты. Сохраняет HL/DE/BC, разрушает AF.
printPanelFile:
        ex      de, hl
        call    setCursorPosPanel
        ex      de, hl
        push    hl
        call    indexedFile
    IF NC_FILE_COLORS
        call ncPaintFileColor
    ENDIF
        call    printFileNamOnPanel
        pop     hl
        ret

    IF NC_BITMAP_SCROLL
; The selection stays at the same screen coordinates. A color scroll copied
; the old bitmap using its normal palette, so only the new name is missing.
printKeptScrollFile:
    IF ENABLE_COLOR
        COLOR   COLOR_CURSOR
        call    printPanelFile
        COLOR   COLOR_FILE
        ret
    ELSE
        ; The excluded inverse cell requires restoring the adjacent name.
        push    hl
        push    de
        push    bc
        ld      a, e
        ld      bc, 1
        cp      P_FILE_LIST_Y
        jp z,   pks_Below
        ld      bc, -1
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
        ; По контракту BIOS инверсия — слово FFFFh, а не логический байт 1.
        ld      a, 0FFh
        ld      (mon2_vars.inverse), a
        ld      (mon2_vars.inverse+1), a
        call    printPanelFile
        xor     a
        ld      (mon2_vars.inverse), a
        ld      (mon2_vars.inverse+1), a
        COLOR   COLOR_FILE
        ret
    ENDIF
    ENDIF
