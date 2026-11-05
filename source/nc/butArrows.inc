;----------------------------------------------------------------------------
; MXOS NC.COM
; Клавиши перемещения курсора
;
; 2013-12-18 Дизассемблировано vinxru
; 2022-02-16 Доработано SpaceEngineer
;----------------------------------------------------------------------------

; ---------------------------------------------------------------------------
; На один файл вперед

but_Down:   ; c = количество файлов
            ld      hl, state.panelA_filesCnt
            call    readBytePanel
            ld      c, a

            ; a = файл, на котором курсор
            ld      hl, state.panelA_curFile
            call    readBytePanel

            ; курсор на следующий файл
            inc     a

            ; если курсор на последнем файле + 1, выходим
            cp      c
            jp z,   mainSelectionChanged

            ; иначе сохраняем новое положение курсора
            ; и скроллим панель, если надо
            jp      scrollPanel

; ---------------------------------------------------------------------------
; На один файл назад

but_Up:     ; a = файл, на котором курсор
            ld      hl, state.panelA_curFile
            call    readBytePanel

            ; если курсор на нулевом файле, выходим
            or      a
            jp z,   mainSelectionChanged

            ; курсор на предыдущий файл
            dec     a

            ; иначе сохраняем новое положение курсора
            ; и скроллим панель, если надо
            jp      scrollPanel

; ---------------------------------------------------------------------------
; На FILE_LIST_SIZE/2 файлов вперед

but_Right:  ; c = количество файлов
            ld      hl, state.panelA_filesCnt
            call    readBytePanel
            ld      c, a
            dec     c

            ; a = файл, на котором курсор
            ld      hl, state.panelA_curFile
            call    readBytePanel

            ; увеличиваем на FILE_LIST_SIZE/2 файлов, если не дошли до конца списка
            add     FILE_LIST_SIZE/2
            cp      c

            ; иначе устанавливаем курсор на последний файл
            jp c,   scrollPanel
            ld      a, c
            jp      scrollPanel

; ---------------------------------------------------------------------------
; На FILE_LIST_SIZE/2 файлов назад

but_Left:   ; a = файл, на котором курсор
            ld      hl, state.panelA_curFile
            call    readBytePanel

            ; уменьшаем на FILE_LIST_SIZE/2, если не дошли до 0
            sub     FILE_LIST_SIZE/2

            ; иначе устанавливаем курсор на нулевой файл
            jp p,   scrollPanel
            xor     a
            jp      scrollPanel

; ---------------------------------------------------------------------------
; На первый файл

but_Home:   ld      hl, state.panelA_curFile
            xor     a
            jp      scrollPanel

; ---------------------------------------------------------------------------
; На последний файл

but_End:    ld      hl, state.panelA_filesCnt
            call    readBytePanel
            dec     a
            ld      hl, state.panelA_curFile
            jp      scrollPanel

; ---------------------------------------------------------------------------
; Расчет нового положения указателей и скроллинг панели, если надо

scrollPanel:
            ; сначала сохраняем panelA_curFile
            call    writeBytePanel

            ; c = количество файлов
            ld      hl, state.panelA_filesCnt
            call    readBytePanel
            ld      c, a
            dec     c

            ; b = файл, с которого начинается спосок файлов панели
            ld      hl, state.panelA_firstFile
            call    readBytePanel
            ld      b, a

            ; a = файл, на котором курсор
            ld      hl, state.panelA_curFile
            call    readBytePanel

            ; a = положение курсора относительно начала списка файлов панели
            sub     b

            ; если курсор выше первого файла панели, скроллим вниз
            jp c,   scrollDown

            ; если курсор ниже последнего файла панели, скроллим вверх
            sub     FILE_LIST_SIZE
            jp nc,  scrollUp

            ; иначе выходим
            jp      mainSelectionChanged

scrollDown: ; скроллинг списка вниз
            add     a, b

            ; если скроллить некуда, выходим
            jp nc,  mainSelectionChanged

            ; направление перерисовки списока файлов - сверху вниз
            ld      b, 2
            jp      scrollExit

scrollUp:   ; скроллинг списка вверх
            inc     b
            add     a, b

            ; если скроллить некуда, выходим
            cp      c       ; c = количество файлов
            jp nc,  mainSelectionChanged

            ; направление перерисовки списока файлов - снизу вверх
            ld      b, 1

scrollExit: ; сохраняем новое значение panelA_firstFile
            ld      hl, state.panelA_firstFile
    IF NC_BITMAP_SCROLL
            ; Модуль шага по CF unsigned-вычитания: Home/End могут
            ; перескочить больше 127 файлов. B сохраняет направление.
            ld      d, a
            call    readBytePanel
            ld      e, a
            ld      a, d
            sub     e
            jp nc,  scrollDeltaReady
            cpl
            inc     a
scrollDeltaReady:
            ld      (vars.scrollRows), a
            ld      a, d
    ENDIF
            call    writeBytePanel

    IF NC_BITMAP_SCROLL
            call    scrollBitmapPanel
            jp c,   scrollFinished
    ENDIF
            ; перерисовываем только список файлов
            ; b = направление
            call    printFilePanel

scrollFinished:
            ; Курсор уже нарисован после первой строки списка.
            call    printInfoLine
            jp      mainLoop_cursorReady

; ---------------------------------------------------------------------------
