;----------------------------------------------------------------------------
; MXOS NC.COM
; Вывод инфомарции о файле внизу окна
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

printInfoLine:
        ; Если файлов нет, выходим
        ld      hl, state.panelA_filesCnt
        call    readBytePanel
        and     a
        jp z,printInfoEmpty

        COLOR   0F1h

        ld      hl, (P_FILE_NAME_X << 8) + P_FILE_NAME_Y
        call    setCursorPosPanel

        ; ld      c, '\'
        ; call    bios_printChar

        ; Устанавливаем hl на описатель файла
        call    getSelectedFile
        push    hl

        ; Путь выбранного файла: целые компоненты, с резервом под размер.
        call    buildCachedSelectedPath
        jp nc,  printInfoPathReady
        ld      hl, vars.tempFileDescr
printInfoPathReady:
        ld      b, 23
        call    compactPath
        ld      b, 23
        ld      c, 1
        call    printString2
        ld      a, b
        or      a
        call nz,printSpaces

        ; Смещаем курсор
        ld      hl, (P_FILE_DATA_X << 8) + P_FILE_DATA_Y
        call    setCursorPosPanel

        ; Адрес не является метаданными FAT. Печатать только точный размер.
        ; Устанавливаем hl на поле размера файла
        pop     hl
        push    hl
        ld      bc, FILE_INFO.attrib
        add     hl, bc
        ld      a, (hl)
        and     010h
        pop     hl
        jp nz,  printInfoDirectory
        ld      bc, FILE_INFO.size ; bc = смещение
        add     hl, bc

        ; Читаем размер файла в de и печатаем в десятичной форме
        ld      e, (hl)
        inc     hl
        ld      d, (hl)
        ld      b, 20h
        jp      bios_printDecWord

printInfoDirectory:
        ld hl,infoDirectoryText
        jp bios_printString
infoDirectoryText: DB "<DIR>",0

printInfoEmpty:
        COLOR 0F1h
        ld hl,(P_FILE_NAME_X << 8) + P_FILE_NAME_Y
        call setCursorPosPanel
        ld b,29
        jp printSpaces
