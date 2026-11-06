;----------------------------------------------------------------------------
; MXOS NC.COM
; Основной цикл
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

mainReload: ; Сначала обновляем приглашение активной панели.
            call mainPrepareReload
            ; Затем перерисовываем обе панели.
            ld      a, (state.activePanel)
            and     a
            jp      nz, mainReload_0
            call    loadAndPrintA
            call    loadAndPrintB
            jp      mainPanelsReady

; Смена контекста не затрагивает другую файловую панель.
mainReloadActive:
            call mainPrepareReload
            ld a,(state.activePanel)
            ld c,a
            call loadAndPrintPanel
            ld hl,state.panelB_info
            ld a,(state.activePanel)
            or a
            jp z,mainOtherPanelInfo
            dec hl
mainOtherPanelInfo:
            ld a,(hl)
            or a
            jp z,mainPanelsReady
            ld a,(state.activePanel)
            xor 1
            ld c,a
            call loadAndPrintPanel
            jp mainPanelsReady
mainPrepareReload:
            ld hl,state.panelA_drive
            call readBytePanel
            ld e,1
            MXCALL MX_DOS_FILESELECTDRIVE
            call restorePanelDirectory
            call updateCommandPath
            call redrawCommandLine
            ret

mainReload_0:
            call    loadAndPrintB
            call    loadAndPrintA
            
mainPanelsReady:
            xor a
            ld (ncDirtyMask),a
            ld (ncDamageMask),a
            ld (ncSelectiveRefresh),a
            ld (ncKeepFrames),a
            ; Заголовки использовали path как временный буфер.
            call updateCommandPath
            jp mainSelectionChanged
mainDriveChanged:
            call    updateCommandPath
            call    redrawCommandLine

mainSelectionChanged:
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
            call    ncReadKey

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
            cp      8Bh   ; F10 — самостоятельный CLI
            jp z,   ncExitToCli
            cp      8Ah   ; F9 (8Ah)
            jp z,   mainLoop_editDone
mainLoop_e: cp      20h   ; Пробел (20h)
            jp c,   mainLoop_editDone

            ; Вывод символов в ком строку
            ld      hl, (vars.cmdLineEnd)
            ld      c, a

            ; Если ком строка переполнена, выходим
            ld      a, vars.cmdLineCtrl & 0FFh
            cp      l
            jp z,   mainLoop_editDone
            ld      (hl), c

            ; Увеличиваем длину
            inc     hl
            ld      (vars.cmdLineEnd), hl
            ld      (hl), 0

            ; Выводим символ на экран
            ld      a, (vars.cmdCapacity)
            ld      b, a
            ld      a, l
            sub     vars.cmdLine & 0FFh
            cp      b
            jp c,   mainLoop_appendChar
            jp z,   mainLoop_appendChar
            call    redrawCommandText
            jp      mainLoop_editDone
mainLoop_appendChar:
            call    bios_printChar
            jp      mainLoop_savePos

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
            ld      (hl), 0
            ld      a, (vars.cmdOffset)
            or      a
            jp z,   mainLoop_eraseChar
            call    redrawCommandText
            jp      mainLoop_editDone
mainLoop_eraseChar:
            ld      hl, asc_DC17
            call    bios_printString
            jp      mainLoop_savePos

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

; F10 завершает панельную оболочку, сохраняя её контексты.
ncExitToCli:
            call ncStopObserver
            call saveState
            ld hl,state.panelA_drive
            call readBytePanel
            MXCALL MX_DOS_FILESELECTDRIVE
            call restorePanelDirectory
            ld a,1
            MXCALL MX_DOS_SETSHELL
            COLOR COLOR_CMDSCREEN
            ld c,01Fh
            call bios_printChar
            jp bios_reboot
