;+---------------------------------------------------------------------------
; MXOS
; Запустить файл
;
; На входе
;  hl - ком строка в формате [Диск:]файл[ аргументы]
;
; На выходе
;  сf - ошибка
;
; 2013-12-12 Дизассемблировано vinxru
;----------------------------------------------------------------------------

fileExec:   ; Сюда будем записывать результат
            ld      de, CMD_LINE
            
            ; Если первый символ меньше пробела, выходим c CF
            ld      a, (hl)
            cp      ' '
            jp      c, stc_ret

            ; Максимальный размер ком строки (+2 терминатора, итого 82h)
            ld      c, 80h        

fileExec_0: ; Если символ меньше пробела, выходим
            ld      a, (hl)
            cp      20h
            jp c,   fileExec_1

            ; Копируем симовол из hl в de
            ld      (de),a
            inc     hl
            inc     de

            ; Повторяем 80h раз
            dec     c
            jp nz,  fileExec_0

fileExec_1: ; В конец имени помещаем 13, 0
            ld      a, 0Dh
            ld      (de), a
            inc     de
            xor     a
            ld      (de), a

            ; Выводим на экран 0Ah
            call    printCharA

            ; Выводим на экран имя файла
            ld      hl, CMD_LINE
            call    j_printString

            ; Подготовка имени файла
            ld      hl, CMD_LINE
            ld      de, v_curFileDescr
            call    fileNamePrepare

            ; Запоминаем в стеке текущий адрес командной строки (начало списка аргументов)
            push    hl

            ; Если это BAT файл
            ld      de, aBat ; "BAT"
            call    cmpFileExt
            jp z,   execBat

            ; Если это COM или EXE файл
            call    cmpFileExt
            jp z,   execExeCom
            call    cmpFileExt
            jp nz,  execError

execExeCom:
            ; Вытаскиваем из стека текущий адрес командной строки (начало списка аргументов)
            pop     de

            ; Загрузить файл
            ld      hl, v_curFileDescr
            call    fileLoad
            jp c,   badCommand

            ; Адрес командной строки
            push    de

            ; Выводим на экран 0Ah
            ;call printCharA

            ; hl = указатель на адрес загрузки (= адрес запуска) файла
            ld      de, FILE_DESCRIPTOR.loadAddress   ; смещение поля с адресом загрузки
            add     hl, de

            ; de = адрес загрузки (= адрес запуска) файла
            ld      e, (hl)
            inc     hl
            ld      d, (hl)

            ; Адрес возврата
            ld      hl, defAppReturn
            ex      (sp), hl

            ; Запуск программы (переход на de)
            ex      de, hl  ; hl <--> de
            jp      (hl)    ; pc <-- hl

; ---------------------------------------------------------------------------

defAppReturn:
            or  a
            ret

; ---------------------------------------------------------------------------

execError:
            pop     de
badCommand:
            ld      hl, txtBadCommand; "\nBAD COMMAND or FILE NAME"
            call    j_printString
stc_ret:
            scf
            ret

; ---------------------------------------------------------------------------

printCharA:
            ld  c, 0Ah
            jp  j_printChar

