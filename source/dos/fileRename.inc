;+---------------------------------------------------------------------------
; MXOS
; Переименовать файл в пределах текущего диска.
;
; Вход:
;   hl - старое имя;
;   de - новое имя.
; Выход:
;   CF - ошибка;
;   bc, de, hl разрушаются.
;
; Важно: поиск нового имени может вытеснить сектор исходного дескриптора из
; кэша. Поэтому перед записью имени исходный файл ищется повторно.
;----------------------------------------------------------------------------

fatFileRename:
            xor     a
            ld      (bios_vars.diskWriteErr), a
            ; Служебные записи . и .. не переименовываются.
            ld      a,(de)
            cp      02Eh
            jp z,   blockArgument
            ld      a,(hl)
            cp      02Eh
            jp z,   blockArgument
            ; Найти исходный файл или каталог.
            call    fileFindAny
            ret c

            call    fileCheckDeleteAllowed
            ret c

            ; Совпадение без учёта регистра разрешает обновить написание.
            push    hl
            push    de
            ld      c, FILE_NAME_LENGTH + 3
            call    stricmp
            pop     de
            pop     hl

            ; Сохранить входные регистры для общего выхода popDBH_ret.
            push    hl
            push    bc
            push    de

            ; Существующее назначение не перезаписываем. При смене только
            ; регистра это та же запись; повторно ищем её перед записью.
            ex      de, hl
            jp z,   fileRename_noExisting
            call    fileFindAny
            jp c,   fileRename_missing
            ld      a,DOS_ERR_EXISTS
            call    dosSetError
            jp      popDBH_ret
fileRename_missing:
            ld a,(v_dosError)
            cp DOS_ERR_NOT_FOUND
            jp nz,stc_popDBH_ret
fileRename_noExisting:

            ; Повторно найти исходный файл: предыдущий поиск мог вытеснить
            ; его сектор и сделать v_cachedDescrPtr недействительным.
            ex      de, hl                  ; hl = старое, de = новое
            call    fileFindAny
            jp c,   popDBH_ret

            ; hl = новое имя, de = актуальный дескриптор исходного файла.
            ld      hl, (v_cachedDescrPtr)
            ex      de, hl

            ld      c, FILE_NAME_LENGTH + 3
fileRenameCopyName:
            ld      a, (hl)
            ld      (de), a
            inc     hl
            inc     de
            dec     c
            jp nz,  fileRenameCopyName

            ld      hl,(v_cachedDescrPtr)
            call    fatEncodeName
            call    markDirSectorAsModified
            jp c,   popDBH_ret
            call    saveSectorCache
            call    fatFinishWrite
            jp      popDBH_ret
