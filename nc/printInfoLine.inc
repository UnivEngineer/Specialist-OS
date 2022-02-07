;----------------------------------------------------------------------------
; MXOS NC.COM
; Вывод инфомарции о файле внизу окна
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

printInfoLine:
        ; Если файлов нет, выходим
        ld      hl, panelA_filesCnt
        call    readBytePanel
        and     a
        ret z

        COLOR   COLOR_INFOLINE

        ld      hl, (P_FILE_NAME_X << 8) + P_FILE_NAME_Y
        call    setCursorPosPanel

        ; ld      c, '\'
        ; call    bios_printChar

        ; Устанавливаем hl на дескриптор файла
        call    getSelectedFile
        push    hl

        ; Печатаем имя файла в компактном виде
        ld      de, nc_vars.tempFileDescr  ; буфер
        call    compactName
        ld      hl, nc_vars.tempFileDescr
        ld      b, DIR_NAME_LENGTH + 6  ; макс длина строки (name.ext + ' ' + терминатор)
        ld      c, 20                   ; терминатор - пробел или меньше
        call    printString2

        ; После имени файла, если оно короткие, надо напечатать
        ; пробелы, чтобы стереть предыдущий текст. После работы
        ; printString2 у нас в b как раз количество символов,
        ; оставшихся до DIR_NAME_LENGTH + 6
		call    printSpaces	; b - кол-во пробелов

        ; Смещаем курсор
        ld      hl, (P_FILE_DATA_X << 8) + P_FILE_DATA_Y
        call    setCursorPosPanel

        ; Устанавливаем hl на поле адреса загрузки
        pop     hl
        push    hl
        ld      bc, FILE_DESCRIPTOR.loadAddress + 1 ; bc = смещение
        add     hl, bc

        ; Читаем старший и младший байты адреса загрузки
        ; и тут же печатаем их в HEX форме
        ld      a, (hl)
        call    bios_printHexByte
        dec     hl
        ld      a, (hl)
        call    bios_printHexByte

        ; Смещаем курсор
		LD 	    c, 18h  ; код символа "вправо"
		call	bios_printChar

        ; Устанавливаем hl на поле размера файла
        pop     hl
        ld      bc, FILE_DESCRIPTOR.size ; bc = смещение
        add     hl, bc

        ; Читаем размер файла в de и печатаем в десятичной форме
        ld      e, (hl)
        inc     hl
        ld      d, (hl)
        inc     de          ; в дескрипторе файла хранится размер-1
        ld      b, 20h
        jp      bios_printDecWord
