; Файловые клавиши и вход с аргументом командной строки
; Порядок включения задан главным .asm; см. README.md.

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
        JP    restartAfterLoad ; ОШИБКА: адрес лежит в перезаписываемом буфере строки

keyHandler_F5:
        CALL  inputFileName
        CALL  saveFile
        JP    synchronizeTopAndRedraw

keyHandler_F6:
        CALL  inputFileName
        CALL  insertFile
        JP    restartAfterLoad ; ОШИБКА: адрес лежит в перезаписываемом буфере строки

;----------------------------------------------------------------------

        ; Сохранённый байт выравнивания исходной версии.
        NOP

prepareFilePrompt:
        CALL  commitLineEdit
        LD    HL,FILE_NAME_HISTORY
        RET
; Проверка до загрузки: fileLoad2 пишет полные секторы и не знает границ редактора.
; Выход: CF=1 — файл не помещается в буфер 01800h-084FFh.
; Разрушает AF, DE, HL, флаги; BC сохраняет.
; Учитывает верхнее слово размера, но не суммарный размер добавления F6.
editorCheckFileSize:
    IF !STANDARD_EDITOR
        LD    HL,(fileDescSize + 2)
        LD    A,H
        OR    L
        JP    NZ,editorCheckFileSizeError
    ENDIF
        LD    HL,(fileDescSize)
        LD    DE,EDITOR_BUFFER_SIZE
        CALL  bios_cmp_hl_de
        JP    NC,editorCheckFileSizeError
        OR    A
        RET

editorCheckFileSizeError:
        SCF
        RET

editorShowFileTooLarge:
        LD    HL,txtOutOfMemory
        CALL  bios_printString
        JP    bios_getch

openCommandLineFile:
        EX    DE,HL
        LD    DE,fileDescName
        CALL  bios_fileNamePrepare
        LD    HL,fileDescName
        CALL  bios_fileLoadInfo
        JP    C,startupBannerAndLineBuffer
        CALL  editorCheckFileSize
        JP    C,commandLineFileTooLarge
        LD    HL,(v_textBeginLo)
        EX    DE,HL
        LD    HL,fileDescName
        CALL  bios_fileLoad2
        JP    startupBannerAndLineBuffer

commandLineFileTooLarge:
        CALL  editorShowFileTooLarge
        JP    bios_reboot

        DB    00H,00H
