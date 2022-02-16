;----------------------------------------------------------------------------
; MXOS NC.COM
; Клавиша F7. Загрузка файла с ленты на накопитель
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

; Сохранение sp   и установка обр   ошибок.

prepareTapeLoad:
            ld          hl, 0
            add         hl, sp
            inc         hl
            inc         hl
            ld          (vars.savedSP), hl

            ld          hl, tapeErrorHandler
            ld          (bios_vars.tapeError), hl

            ld          a, 0Fh
            ld          (IO_KEYB_C),a
            ld          a, 0FEh
            ld          (IO_KEYB_A),a
            ret

; ---------------------------------------------------------------------------

but_F7:     ; Рисуем окно
            call        drawWindow1

            ; Выводим "LOADING FROM TAPE TO A:"
            CENTER_LINE 23, 5Eh
            ld          hl, aLoadingFromTapeTo
            call        bios_printString       
            call        printSelDrive
            
            ; Сохранение sp и установка обработчика ошибок
            call        prepareTapeLoad   

loc_D2AA:
            ; Ждем пилот и D9 D9 D9
            ld          a, 0FFh           
            ld          c, 3
            jp          loc_D2B3
loc_D2B1:
            ld          a, 8
loc_D2B3:
            call        bios_tapeRead
            cp          0D9h 
            jp nz,      loc_D2AA
            dec c
            jp nz,      loc_D2B1

            ; Читаем имя
            ld          hl, vars.tempFileDescr
            ld          d, h
            ld          e, l
            ld          c, DIR_NAME_LENGTH + 3 ; старый формат файла 9 символов, новый - DIR_NAME_LENGTH + 3 символов
loc_D2C6:
            ld          a, 8
            call        bios_tapeRead
            cp          ' '
            jp c,       tapeReadAppendName
            ld          (hl), a
            inc         hl
            dec         c
            jp nz,      loc_D2C6

loc_D2D6:
            ld         (hl), 0 ; обнуляем байт атрибута в дескрипторе файла

            call        printInvFile

            ; Пропускаем пилот и читаем начальный адрес загрузки
            ld          a, 0FFh
            call        tapeReadWord2
            ld          (vars.tapeLoadAddr), hl

            ; Читаем конечный адрес загрузки
            ex          de, hl
            call        tapeReadWord

            ; Считаем размер файла (hl = hl - de)
            ld          a, l
            sub         e
            ld          l, a
            ld          a, h
            sbc         d
            ld          h, a
            ld          (vars.tempFileDescr.size), hl

            ; Читаем данные по адресу 0000
            inc         hl
            ex          de, hl
            ld          hl, 0
            ld          (vars.tempFileDescr.loadAddress), hl           
loc_D2F8:   ld          a, 8
            call        bios_tapeRead
            ld          (hl), a
            inc         hl
            dec         de
            ld          a, d
            or          e
            jp nz,      loc_D2F8

            ; Чиаем CRC
            call        tapeReadWord
            ld          (vars.tapeSaveCRC), hl

            ; Считаем CRC
            ld          hl, (vars.tempFileDescr.size)
            ex          de, hl
            ld          hl, 0
            add         hl, de
            ex          de, hl
            ld          hl, 0
            call        bios_calcCS
            ld          hl, (vars.tapeSaveCRC)

            ; Если расчетный CRC не равен реальному, выходим
            ld          a, b
            cp          h
            jp nz,      F7_crcError
            ld          a, c
            cp          l
            jp nz,      F7_crcError

            ; Соханяем файл на текущий диск
            call        F7_saveFile

            ; Перерисовываем экран и продолжаем загрузку
            call        loadAndPrintA
            call        loadAndPrintB
            jp          but_F7

; ---------------------------------------------------------------------------

F7_saveFile:
            ld          hl, (vars.tapeLoadAddr)
            push        hl
            ld          hl, vars.tempFileDescr
            jp          saveFileInt

; ---------------------------------------------------------------------------

F7_crcError:
            call        drawWindow2

            ; Вывод "ERROR LOADING FROM TAPE"
            COLOR       COLOR_DIALOG_ERR
            CENTER_LINE 23, 80h
            ld          hl, aErrorLoadingTa
            call        bios_printString

            ; Вывод строки из vars.input
            COLOR       COLOR_DIALOG
            ld          hl, 5090h
            ld          (bios_vars.cursorY), hl
            ld          hl, vars.input
            call        printStringInv

loc_D358:   ; Ждем нажатия ESC
            call        bios_keyScan
            cp          1Bh
            jp nz,      loc_D358

            jp          mainReload

;----------------------------------------------------------------------------
; Чтение слова с ленты
;----------------------------------------------------------------------------

tapeReadWord:
            ld          a, 8
tapeReadWord2:
            call        bios_tapeRead
            ld          l, a
            ld          a, 8
            call        bios_tapeRead
            ld          h, a
            ret


; ---------------------------------------------------------------------------

; Если имя файла на ленте было короче, чем DIR_NAME_LENGTH+3, дополняем его пробелами
tapeReadAppendName:
            ld          (hl), ' '
            inc         hl
            dec         c
            jp nz,      tapeReadAppendName
            jp          loc_D2D6

