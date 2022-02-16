;----------------------------------------------------------------------------
; MXOS NC.COM
; Клавиша F9 - Сохранение файла на магнитофон
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

but_F9:     ; Рисуем окно
            call    drawWindowIfSel

            ; Выводим "SAVING FROM A: TO TAPE"
            CENTER_LINE 20, 5Eh
            ld      hl, aSaveFromToTape
            call    bios_printString
            call    printSelDrive

            ; ?
            call    printInvSelFile

            ; Ждем нажатия Enter или ESC.
loc_D39C:   call    bios_keyScan
            cp      1Bh
            jp z,   mainReload
            cp      0Dh
            jp nz,  loc_D39C

            ; Сохранение sp   и установка обр   ошибок.
            call    prepareTapeLoad

            call    drawWindowIfSel

            ; Выводим "SAVING TO TAPE"
            CENTER_LINE 14, 5Eh
            ld      hl, aSavingToTape 
            call    bios_printString

            ; Загрузка файла на адрес 0
            call    loadSelFileAt0  ; на выходе de - оригинальный адрес загрузки

            ; Получение адреса загрузки и размера файла
            push    de  ; оригинальный адрес загрузки
            push    hl  ; начало дескриптора
            ld      de, FILE_DESCRIPTOR.size
            add     hl, de
            ld      e, (hl)
            inc     hl
            ld      d, (hl) ; de = размер-1
            pop     hl  ; начало дескриптора
            push    de  ; размер
            push    hl  ; начало дескриптора
            ld      hl, 0   ; файл загружен на адрес 0
            add     hl, de  ; hl = размер-1 + 0
            ex      de, hl  ; de = размер-1 + 0
            ld      hl, 0

            ; Расчет контрольной суммы от hl до de
            call    bios_calcCS ; bc = crc
            ld      h, b
            ld      l, c
            ld      (vars.tapeSaveCRC), hl

            ; Пилот-тон
            call    tapeWritePilot  ; 200h нулей, затем E6h

            ; Начало файла
            ld      a, 0D9h
            call    bios_tapeWrite  ; три раза 0D9h
            call    bios_tapeWrite
            call    bios_tapeWrite

            ; имя файла
            pop     hl ; начало дескриптра
            ld      bc, DIR_NAME_LENGTH+2 ; длина (запишется +1); старый формат - 9 символов, новый - DIR_NAME_LENGTH+3
            call    tapeWriteBlock

            ; Пилот-тон
            call    tapeWritePilot  ; 200h нулей, затем E6h

            pop     bc ; размер
            pop     hl ; оригинальный адрес загрузки
            call    tapeWriteWord

            add     hl, bc ; конечный адрес загрузки
            call    tapeWriteWord

            ; Запись файла
            ld      hl, 0
            call    tapeWriteBlock    ; hl = адрес, bc = размер-1

            ; Запись контрольной суммы
            ld      hl, (vars.tapeSaveCRC)
            call    tapeWriteWord

            jp      mainReload

