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

            ; Запомнить текущий диск в регистре b
            ld      e, 0
            call    bios_fileGetSetDrive
            ld      b, a

            ; Подготовить имя файла A:NC.EXT
            ld      hl, aNcExt
            ld      de, vars.tempFileDescr; vars.tempFileDescr используется как буфер
            call    bios_fileNamePrepare
            ex      de, hl

            ; Загрузить файл A:NC.EXT по адресу 0
            ld      hl, vars.tempFileDescr
            ld      de, 0
            ld      c, 1
            call    bios_fileGetSetAddr ; изменить адрес загрузки файла на de = 0
            call    bios_fileLoad
            jp c,   Enter_stdExec

            ; Переключить диск обратно
            ;mov    a, b
            ;mvi    e, 1
            ;call   bios_fileGetSetDrive

            ; Получить имя выбранного файла
            call    getSelectedFile
            ld      a, (hl)
            inc     a
            jp z,   mainLoop ; Ничего не выбрано

            ; hl = указатель на расширение файла
            ld      de, FILE_DESCRIPTOR.ext
            add     hl, de

            ld      de, 0

            ; Сравниваем с расширеним из файла,
            ; нечувствительно к регистру
Enter_extLoop:
            push    hl
            ld      c, 3
            call    bios_stricmp
            jp nz,  Enter_nextExt

            ; Найдено расширение
            pop     hl
            inc     de

            ; Копирование имени выбранного файла в ком строку
Enter_5:
            ld      hl, vars.cmdLine
Enter_4:
            ld      a, (de)
            ld      (hl),a
            cp      0Dh
            jp z,   Enter_3
            inc     a
            jp z,   Enter_3
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
            ld      hl, vars.cmdLine
            jp      bios_fileExec

; ---------------------------------------------------------------------------

Enter_nextExt:
            pop     hl

Enter_2:
            ld      a, (de)
            inc     de
            cp      0Dh
            jp z,   Enter_extLoop
            inc     a
            jp      nz, Enter_2
            jp      Enter_stdExec
