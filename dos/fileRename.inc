;+---------------------------------------------------------------------------
; MXOS
; Переименовать файл
;
; На входе 
;  hl - исходное имя
;  de - новое имя
;
; На выходе
;  cf - ошибка
;  bc, de, hl - сохраняются
;
; 2013-12-12 Дизассемблировано vinxru
;----------------------------------------------------------------------------

fileRename: ; Найти файл с именем из hl
            call    fileFind
            ret c 

            ; Сохранить регистры
            push    hl
            push    bc
            push    de
            
            ld      hl, (v_cachedDescrPtr)

            ; Попытаться найти файл с новым именем
            ex      de, hl
            call    fileFind

            ; Если такой файл есть, то удалить
            call nc,deleteFileInt
            
            ; Заменяем имя
            ld      c, DIR_NAME_LENGTH + 3        
loc_CD2C:   ld      a, (hl)
            ld      (de),a
            inc     hl
            inc     de
            dec     c
            jp nz,  loc_CD2C

            ; Ставим сектору каталога флаг isModified
            call    markDirSectorAsModified

            ; Сохраняем изменения на диск
            call    saveSectorCache

            ; Выход
            or      a
            jp      popDBH_ret

