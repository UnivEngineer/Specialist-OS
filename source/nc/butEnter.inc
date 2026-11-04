;----------------------------------------------------------------------------
; MXOS NC.COM
; Клавиша ENTER
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

but_Enter:  ; Если ком строка не пустая
            ld      hl, (vars.cmdLineEnd)
            ld      a, vars.cmdLine & 0FFh
            cp      l
            jp nz,  EnterCmdLine

            ; Получить имя выбранного файла
            call    getSelectedFile
            ld      a, (hl)
            inc     a
            jp z,   mainLoop ; Ничего не выбрано

            ; hl = указатель на расширение файла
            ld      de, FILE_DESCRIPTOR.ext
            add     hl, de

            ; Ассоциации — общий сервис DOS. Буфер временный, после кода NC.
            push    hl
            call    getCopyBufferSize
            ld      b,h
            ld      c,l
            pop     hl
            jp c,   Enter_stdExec
            ld      de, COPY_BUFFER
            call    bios_fileAssociation
            jp c,   Enter_stdExec
            ex      de, hl

Enter_5:
            ; Оставляем место для пробела, полного пути 8.3 и терминатора.
            ld      b, 45
            ld      hl, vars.cmdLine
Enter_4:
            ld      a, (de)
            ld      (hl), a
            cp      0Dh
            jp z,   Enter_3
            inc     a
            jp z,   Enter_3
            dec     b
            jp z,   Enter_stdExec
            inc     hl
            inc     de
            jp      Enter_4

; ---------------------------------------------------------------------------

Enter_3:
            ld      (hl), ' '
            inc     hl
            ld      (hl), 0
            ld      (vars.cmdLineEnd), hl

            ; Вывод командной строки на экран
            COLOR   COLOR_CMDSCREEN
            ld      hl, vars.cmdLine
            call    bios_printString

Enter_stdExec:
            ; Получить выбранный файл
            call     getSelectedFile

            ; Ничего не выбрано
            ld      a, (hl)
            inc     a
            jp z,   mainLoop

            ; ?
            call    selFileToCmdLine

EnterCmdLine:
            ; Команда X: без имени переключает активную панель на диск X.
            ld      hl, (vars.cmdLineEnd)
            ld      de, vars.cmdLine + 2
            call    bios_cmp_hl_de
            jp nz,  EnterExec

            ld      a, (vars.cmdLine + 1)
            cp      ':'
            jp nz,  EnterExec

            ld      a, (vars.cmdLine)
            and     0DFh
            sub     'A'
            cp      8
            jp nc,  EnterExec
            ld      c, a

            ; Не переключаться на букву без установленного драйвера.
            push    bc
            call    buildDriveList
            pop     bc
            call    findDrive
            jp c,   EnterExec

            ld      a, c
            ld      hl, state.panelA_drive
            call    writeBytePanel
            xor     a
            ld      hl, state.panelA_info
            call    writeBytePanel
            ld      hl, state.panelA_firstFile
            call    writeBytePanel
            ld      hl, state.panelA_curFile
            call    writeBytePanel

            call    clearCmdLine
            call    saveState
            jp      mainReload

EnterExec:
            ; Очистка экрана
            COLOR   COLOR_CMDSCREEN
            ld      c, 1Fh
            call    bios_printChar

            ; Помещаем в конец ком строки 0Dh
            ld      hl, (vars.cmdLineEnd)
            ld      (hl), 0Dh

            ; Сохраняем состояние
            call    saveState

            ; Запускаем файл
            ; Текущий том команды — активная панель, а не последний прочитанный каталог.
            ld      hl, state.panelA_drive
            call    readBytePanel
            ld      e, 1
            call    bios_fileSelectDrive
            ld      hl, vars.cmdLine
            jp      bios_fileExec
