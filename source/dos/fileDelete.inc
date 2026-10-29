;+---------------------------------------------------------------------------
; MXOS
; Удаление файла
;
; На входе
;  hl - имя файла
;
; На выходе
;  cf - ошибка
;  bc, de, hl - сохраняются
;
; 2013-12-12 Дизассемблировано vinxru
; 2022-02-11 Переработано SpaceEngineer
;----------------------------------------------------------------------------

fatFileDelete: ; Найти файл с именем из hl
            xor     a
            ld      (bios_vars.diskWriteErr), a
            call    fileFind
            ret     c 

            ; Флаги System и Read-only запрещают удаление.
            call    fileCheckDeleteAllowed
            ret c

            ; Удалить файл
            call    deleteFileInt

            ; Сохранить изменения на диск
            call    saveSectorCache

            ; Результат
            jp      fatFinishWrite
            
; ---------------------------------------------------------------------------

; Проверить атрибуты найденной записи. CF=1: удаление запрещено.
; AF разрушается; BC, DE и HL сохраняются.
fileCheckDeleteAllowed:
            ld      a, (v_tmpFileDescr + FILE_DESCRIPTOR.attrib)
            and     005h
            ret z
            scf
            ret

deleteFileInt:
            ; Сохраняем регистры
            push    hl
            push    bc
            push    de

            ; Помечаем файл в кэше, как удаленный, заменяя первую букву его имени на 0FFh
            ld      hl, (v_cachedDescrPtr)
            ld      (hl), 0FFh
            push    hl

            ; Ставим сектору каталога флаг isModified
            call    markDirSectorAsModified

            ; Получаем первый кластер файла в de
            pop     hl
            ld      de, FILE_DESCRIPTOR.firstCluster
            add     hl, de
            ld      e, (hl)
            inc     hl
            ld      d, (hl)

deleteFileLoop:
            ld      h, d
            ld      l, e

            ; Обнуляем кластер номер de и получаем номер
            ; следующиего кластера файла в de
            call    fatFreeCluster

            ; Если это был не последний кластер, продолжаем.
            ld      a, d
            cp      h
            jp nz,  deleteFileLoop
            ld      a, e
            cp      l
            jp nz,  deleteFileLoop

            ; Восстаналиваем регистры и выходим
            jp      popDBH_ret

; Завершение изменения файла после сохранения кэша.
; Вход: diskWriteErr от аппаратных записей, в том числе при вытеснении.
; Выход: CF=1 при отказе записи, CF=0 при успехе; AF разрушается.
; BC, DE, HL сохраняются. При отказе сбрасываем неподтверждённый кэш:
; последующее чтение должно отражать носитель, а не неудачную операцию.
; Уже записанные на носитель секторы это не откатывает.
fatFinishWrite:
            ld      a, (bios_vars.diskWriteErr)
            or      a
            ret z
            push    bc
            push    de
            push    hl
            call    resetSectorCache
            pop     hl
            pop     de
            pop     bc
            scf
            ret

