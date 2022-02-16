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
            jp z,   mainDriveChanged

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
            jp z,   mainDriveChanged

            ; курсор на предыдущий файл
            dec     a

            ; иначе сохраняем новое положение курсора
            ; и скроллим панель, если надо
            jp      scrollPanel

; ---------------------------------------------------------------------------
; На 18 файлов вперед

but_Right:  ; c = количество файлов
            ld      hl, state.panelA_filesCnt
            call    readBytePanel
            ld      c, a
            dec     c

            ; a = файл, на котором курсор
            ld      hl, state.panelA_curFile
            call    readBytePanel

            ; увеличиваем на 18 файлов, если не дошли до конца списка
            add     FILE_LIST_SIZE/2
            cp      c

            ; иначе устанавливаем курсор на последний файл
            jp c,   scrollPanel
            ld      a, c
            jp      scrollPanel

; ---------------------------------------------------------------------------
; На 18 файлов назад

but_Left:   ; a = файл, на котором курсор
            ld      hl, state.panelA_curFile
            call    readBytePanel

            ; уменьшаем на 18, если не дошли до 0
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
            jp      mainDriveChanged

scrollDown: ; скроллинг списка вниз
            add     a, b

            ; если скроллить некуда, выходим
            jp nc,  mainDriveChanged

            ; направление перерисовки списока файлов - сверху вниз
            ld      b, 0
            jp      scrollExit

scrollUp:   ; скроллинг списка вверх
            inc     b
            add     a, b

            ; если скроллить некуда, выходим
            cp      c       ; c = количество файлов
            jp nc,  mainDriveChanged

            ; направление перерисовки списока файлов - снизу вверх
            ld      b, 1

scrollExit: ; сохраняем новое значение panelA_firstFile
            ld      hl, state.panelA_firstFile
            call    writeBytePanel

            ; перерисовываем только список файлов
            ; b = направление
            call    printFilePanel

            ; выходим
            jp      mainDriveChanged

; ---------------------------------------------------------------------------
