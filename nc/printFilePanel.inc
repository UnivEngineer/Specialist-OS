;----------------------------------------------------------------------------
; MXOS NC.COM
; Вывод списка файлов на панель
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

;----------------------------------------------------------------------------
; Перерисовка всей панели

redrawFilePanel:
        COLOR   COLOR_BORDER

        ; Рисование панели
        ld      hl, g_filePanel
        call    draw

        ; Вывод буквы диска
        COLOR   COLOR_INFOLINE
        ld      hl, state.panelA_drive
        call    readBytePanel
        ld      hl, (P_DRIVE_LETTER_X << 8) + P_DRIVE_LETTER_Y
        call    setCursorPosPanel
        call    printDrive

        ; Вывод NAME
        COLOR   COLOR_PANELNAME
        ld      hl, (P_NAME_X << 8) + P_NAME_Y
        call    setCursorPosPanel
        ld      hl, aNameName
        call    bios_printString

        ; Выодим список файлов сверху вниз
        ld      b, 0

;----------------------------------------------------------------------------
; Перерисовка списка файлов
; вход:
;   b - направление (0 - сверху вниз, 1 - снизу вверх)

printFilePanel:
        ; Рисуем сверху вниз или снизу вверх?
        ld      a, b
        or      a
        push    af

        ; Загрузка списка файлов
        ld      hl, state.panelA_drive
        call    readBytePanel   ; a = номер накопителя
        call    loadFiles

        ; Цвет для печати списка файлов
        COLOR   COLOR_FILE

        pop     af
        jp nz,  printFilesDownUp

;----------------------------------------------------------------------------
; Цикл печати сверху вниз

printFilesUpDown:
        ; Указатель списка файлов - на нулевой файл
        ld      hl, FILE_LIST_BUFFER

        ; Положение курсора - на нулевой файл
        ld      de, (P_FILE_LIST_X1 << 8) + P_FILE_LIST_Y

pfud_Loop:
        ; Прерываем цикл по достижении конца списка файлов
        ; (первый символ имени = FF)
        ld      a, (hl)
        inc     a
        ret z

        ; Установка положения курсора
        ex      de, hl
        call    setCursorPosPanel
        ex      de, hl

        ; Печать имени файла
        call    printFileNamOnPanel

        ; Следующий файл
        ld      bc, DIR_DESCR_SIZE
        add     hl, bc

        ; Курсор на следующую строку
        ld      a, e
        add     10
        ld      e, a

        ; Курсор на следующий столбец
        cp      P_FILE_LIST_Y_MAX
        jp c,   pfud_Loop
        ld      a, d
        ld      de, (P_FILE_LIST_X2 << 8) + P_FILE_LIST_Y
        cp      20
        jp c,   pfud_Loop

        ret

;----------------------------------------------------------------------------
; Цикл печати снизу вверх

printFilesDownUp:
        ; Указатель списка файлов - на последний файл
        ld      hl, FILE_LIST_BUFFER + (FILE_LIST_SIZE-1) * DIR_DESCR_SIZE

        ; Положение курсора - на последний файл
        ld      de, (P_FILE_LIST_X2 << 8) + P_FILE_LIST_Y_MAX - 10

pfdu_Loop:
        ; Установка положения курсора
        ex      de, hl
        call    setCursorPosPanel
        ex      de, hl

        ; Печать имени файла
        call    printFileNamOnPanel

        ; Предыдущий файл
        push    bc
        ld      bc, 10000h - DIR_DESCR_SIZE
        add     hl, bc
        pop     bc

        ; Курсор на предыдущую строку
        ld      a, e
        sub     10
        ld      e, a

        ; Курсор на предыдущий столбец
        cp      P_FILE_LIST_Y
        jp nc,  pfdu_Loop
        ld      a, d
        ld      de, (P_FILE_LIST_X1 << 8) + P_FILE_LIST_Y_MAX - 10
        cp      20
        jp nc,  pfdu_Loop

        ret
