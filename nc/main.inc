;----------------------------------------------------------------------------
; MXOS NC.COM
; Основной цикл
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

mainReload: ; Перерисовываем обе панели
            ld    a, (activePanel)
            and   a
            jp nz,      mainReload_0
            call  loadAndPrintA
            call  loadAndPrintB
            jp    mainDriveChanged
mainReload_0:     call  loadAndPrintB
            call  loadAndPrintA
            
mainDriveChanged:
            ; Вывод накопителя в ком строку
            COLOR   COLOR_CMDLINE
            ld      hl, 1F3h
            ld      (bios_vars.cursorY), hl
            ld      hl, panelA_drive
            call    readBytePanel
            call    printDrive

            ; Вывод информации о файле
            call  printInfoLine

mainLoop:   ; Рисуем курсор выбранного файла
            COLOR   COLOR_CURSOR
            call  drawCursor

            ; Положение курсора в ком строке
            ld    hl, (nc_vars.cmdLinePos)
            ld    (bios_vars.cursorY), hl

            ; Ждем нажатия клавиши
            COLOR   COLOR_CMDLINE
            call  bios_getch

            ; Стираем курсор выбранного файла
            push    af
            COLOR   COLOR_FILE
            call    drawCursor
            pop     af

            ; Анализ нажатий клавиши
            ld    c, a
            jp c, mainLoop_ctrl
            and   a
            jp z, F1
            dec   a
            jp z, F2
            dec   a
            jp z, F3
            dec   a
            jp z, F4
            dec   a
            jp z, F5
            dec   a
            jp z, F6
            dec   a
            jp z, F7
            dec   a
            jp z, F8
            dec   a
            jp z, Left
            dec   a
            jp z, Tab
            sub   4
            jp z, Enter
            sub   0Bh
            jp z, Right
            dec   a
            jp z, Up
            dec   a
            jp z, Down
            dec   a
            jp z, Esc
            add   a,1Bh
            cp    7Fh
            jp z, Bkspc
            cp    8Ah
            jp z, f9
mainLoop_e: cp    ' '
            jp c, mainLoop

            ; Вывод символов в ком строку
            ld    hl, (nc_vars.cmdLineEnd)
            ld    c, a
            ld  (hl), a

            ; Если ком строка переполнена, выходим
            ld    a, nc_vars.cmdLineCtrl & 0FFh
            cp    l
            jp z, mainLoop

            ; Увеличиваем длину
            inc   hl
            ld    (nc_vars.cmdLineEnd), hl

            ; Выводим символ на экран
            COLOR   COLOR_CMDLINE
            call  bios_printChar

mainLoop_savePos: ; ?
            ld    hl, (bios_vars.cursorY)
            ld    (nc_vars.cmdLinePos), hl

            jp    mainLoop

; ---------------------------------------------------------------------------

Bkspc:            ; Если ком строка пустая, выходим
            ld    hl, (nc_vars.cmdLineEnd)
            ld    a, nc_vars.cmdLine & 0FFh
            cp    l
            jp z, mainLoop

            ; Удаляем один символ
            dec   hl
            ld    (nc_vars.cmdLineEnd), hl
            
            ; Стираем символ на экране
            COLOR   070h
            ld      hl, asc_DC17
            call    bios_printString

            jp    mainLoop_savePos

; ---------------------------------------------------------------------------

Esc:        call  clearCmdLine
            jp    mainLoop

; ---------------------------------------------------------------------------

ctrlEnter:  ; Если файл не выбран, выход
            call  getSelectedFile
            ld  a, (hl)
            cp    0FFh
            jp z, mainLoop

            call  selFileToCmdLine
            jp    mainLoop

