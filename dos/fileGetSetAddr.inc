;+---------------------------------------------------------------------------
; MXOS
; Получение/изменение адреса загрузки файла
;
; На входе 
;   hl - имя файла
;   с  - 0=чтение, 1=изменение
;   de - адрес
;
; На выходе 
;   de - адрес
;   bc, hl - сохраняются
;
; 2013-12-12 Дизассемблировано vinxru
; 2022-01-31 Доработано SpaceEngineer
;----------------------------------------------------------------------------

fileGetSetAddr:   ; Ищем файл с именем в hl
            call    fileFind
            ret c 

            push    hl

            ; Получаем адрес длины файла
            push    de
            ld      hl, (v_cachedDescrPtr)
            ld      de, FILE_DESCRIPTOR.loadAddress
            add     hl, de
            pop     de

            ; Проверяем режим
            ld      a, c
            dec     c ; ZF=1 если режим 1
            jp nz,  fileGetSetAd_0

            ; Заменяем адрес загрузки
            ld      (hl), e
            inc     hl
            ld      (hl), d

            ; Ставим сектору каталога флаг isModified
            call    markDirSectorAsModified

            ; Сохраняем изменения на диск
            call    saveSectorCache

            ; Выходим
            pop     hl
            or      a
            ret

fileGetSetAd_0:
            ; Читаем адрес загрузки
            ld      e, (hl)
            inc     hl
            ld      d, (hl)

            ; Выходим
            pop     hl
            or      a
            ret

