;----------------------------------------------------------------------------
; MXOS NC.COM
; Вывод списка файлов на панель
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

printFilePanel:
        COLOR   COLOR_BORDER

        ld      hl, g_filePanel
        call    draw

        COLOR   COLOR_PANELNAME

        ; Вывод NAME
        ld      hl, (P_NAME_X << 8) + P_NAME_Y
        call    setCursorPosPanel
        ld      hl, aNameName
        call    bios_printString

        COLOR   COLOR_INFOLINE

        ; Вывод буквы диска
        ld      hl, panelA_drive
        call    readBytePanel
        ld      hl, (P_DRIVE_LETTER_X << 8) + P_DRIVE_LETTER_Y
        call    setCursorPosPanel
        call    printDrive

        COLOR   COLOR_FILE

        ; Вывод всех файлов
        ld      hl, nc_vars.fileListBuffer
        ld      de, (P_FILE_LIST_X1 << 8) + P_FILE_LIST_Y
        ld      c,  0

            
printFilesLoop:
        ; Установка положения курсора
        ex      de, hl
        call    setCursorPosPanel
        ex      de, hl

        ; Файл существует?
        ld      a, (hl)
        inc     a
        jp z,   loc_D94E

        push    bc
        push    de

        ; Печать имени файла
        push    hl
        ld      b, DIR_NAME_LENGTH  ; длина имени
        call    printString2
        pop     hl

        ; Курсор на столбец, где выводятся расширения
        ld      a, d
        add     a, (DIR_NAME_LENGTH + 1) * 3
        ld      d, a
        ex      de, hl
        call    setCursorPosPanel
        ex      de, hl

        ; Печать расширения имени файла
        push    hl
        ld      bc, DIR_NAME_LENGTH  ; длина имени
        add     hl, bc    ; перемещаем hl на начло расширения
        ld      b, 3      ; длина расширения
        call    printString2
        pop     hl

        pop     de

        ; Следующий файл
        ld      bc, DIR_DESCR_SIZE
        add     hl, bc

        pop     bc

        ; Счетчик
        inc     c

        ; Курсор на следующую строку
        ld      a, e
        add     a, 10
        ld      e, a

        ; Курсор на следующий столбец
        cp      P_FILE_LIST_Y_MAX
        jp c,   printFilesLoop
        ld      a, d
        ld      de, (P_FILE_LIST_X2 << 8) + P_FILE_LIST_Y
        cp      20
        jp c,   printFilesLoop

loc_D94E:
        ; Сохранение кол-ва файлов
        ld      hl, panelA_filesCnt
        ld      a, c
        call    writeBytePanel

        ; Если курсор больше максимума
        ld      hl, panelA_curFile
        call    readBytePanel
        cp      c
        ret c 

        ; То изменить на максимум
        dec     a
        ret m 
        jp      writeBytePanel

