;----------------------------------------------------------------------------
; MXOS NC.COM
; Основной цикл
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

mainReload: ; Перерисовываем обе панели
            ld      a, (state.activePanel)
            and     a
            jp      nz, mainReload_0
            call    loadAndPrintA
            call    loadAndPrintB
            jp      mainDriveChanged

mainReload_0:
            call    loadAndPrintB
            call    loadAndPrintA
            
mainDriveChanged:
            ; Вывод накопителя в ком строку
            COLOR   COLOR_CMDLINE
            ld      hl, 1F3h
            ld      (bios_vars.cursorY), hl
            ld      hl, state.panelA_drive
            call    readBytePanel
            call    printDrive

            ; Вывод информации о файле
            call    printInfoLine

mainLoop:   ; Рисуем курсор выбранного файла
            COLOR   COLOR_CURSOR
            call    drawCursor

mainLoop_cursorReady:
            ; Положение курсора в ком строке
            ld      hl, (vars.cmdLinePos)
            ld      (bios_vars.cursorY), hl

            ; Ждем нажатия клавиши
            COLOR   COLOR_CMDLINE
            call    bios_getch

            ; Ввод команды не затрагивает инверсное выделение.
            ; При шаге за край окна старый курсор также можно сохранить.
            push    af
            call    keepCursorForKey
            jp c,   mainLoop_keyKept
            COLOR   COLOR_FILE
            call    drawCursor
mainLoop_keyKept:
            pop     af

            ; Анализ нажатой клавиши
            ld      c, a
            jp c,   but_ctrl
            and     a     ; F1 (00h)
            jp z,   but_F1
            dec     a     ; F2 (01h)
            jp z,   but_F2
            dec     a     ; F3 (02h)
            jp z,   but_F3
            dec     a     ; F4 (03h)
            jp z,   but_F4
            dec     a     ; F5 (04h)
            jp z,   but_F5
            dec     a     ; F6 (05h)
            jp z,   but_F6
            dec     a     ; F7 (06h)
            jp z,   but_F7
            dec     a     ; F8 (07h)
            jp z,   but_F8
            dec     a     ; стрелка влево (08h)
            jp z,   but_Left
            dec     a     ; Tab/Таб (09h)
            jp z,   but_Tab
            dec     a     ; End/ПС (0Ah)
            jp z,   but_End
            sub     2     ; Home/стрелка влево-вверх (0Ch)
            jp z,   but_Home
            dec     a     ; Enter/ВК (0Dh)
            jp z,   but_Enter
            sub     0Bh   ; стрелка вправо (18h)
            jp z,   but_Right
            dec     a     ; стрелка вверх (19h)
            jp z,   but_Up
            dec     a     ; стрелка вниз (1Ah)
            jp z,   but_Down
            dec     a     ; Esc/АР2 (1Bh)
            jp z,   but_Esc
            add     1Bh
            cp      7Fh   ; Del/ЗБ (7Fh)
            jp z,   but_Bkspc
            cp      8Ah   ; F9 (8Ah)
            jp z,   mainLoop_editDone
mainLoop_e: cp      20h   ; Пробел (20h)
            jp c,   mainLoop_editDone

            ; Вывод символов в ком строку
            ld      hl, (vars.cmdLineEnd)
            ld      c, a
            ld      (hl), a

            ; Если ком строка переполнена, выходим
            ld      a, vars.cmdLineCtrl & 0FFh
            cp      l
            jp z,   mainLoop_editDone

            ; Увеличиваем длину
            inc     hl
            ld      (vars.cmdLineEnd), hl

            ; Выводим символ на экран
            COLOR   COLOR_CMDLINE
            call    bios_printChar

mainLoop_savePos: ; ?
            ld      hl, (bios_vars.cursorY)
            ld      (vars.cmdLinePos), hl

            jp      mainLoop_editDone

; ---------------------------------------------------------------------------

but_Bkspc:  ; Если ком строка пустая, выходим
            ld      hl, (vars.cmdLineEnd)
            ld      a, vars.cmdLine & 0FFh
            cp      l
            jp z,   mainLoop_editDone

            ; Удаляем один символ
            dec     hl
            ld      (vars.cmdLineEnd), hl
            
            ; Стираем символ на экране
            COLOR   070h
            ld      hl, asc_DC17
            call    bios_printString

            jp    mainLoop_savePos

; ---------------------------------------------------------------------------

but_Esc:    call    clearCmdLine
            jp      mainLoop_editDone

; ---------------------------------------------------------------------------

but_ctrl:   cp    0Dh
            jp z, ctrlEnter
            jp    mainLoop_e

; ---------------------------------------------------------------------------

ctrlEnter:  ; Если файл не выбран, выход
            call    getSelectedFile
            ld      a, (hl)
            cp      0FFh
            jp z,   mainLoop_editDone

            call    selFileToCmdLine
            jp      mainLoop_editDone


mainLoop_editDone EQU mainLoop_cursorReady
