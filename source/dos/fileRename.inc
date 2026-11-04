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
            ; Найти исходный файл.
            call    fileFind
            ret c

            ; Переименование в то же имя ничего не меняет.
            push    hl
            push    de
            ld      c, FILE_NAME_LENGTH + 3
            call    stricmp
            pop     de
            pop     hl
            ret z

            ; Сохранить входные регистры для общего выхода popDBH_ret.
            push    hl
            push    bc
            push    de

            ; Если файл с новым именем уже существует, удалить его.
            ex      de, hl                  ; hl = новое, de = старое
            call    fileFind
            jp c,   fileRename_missing
            call    fileCheckDeleteAllowed
            jp c,   stc_popDBH_ret
            call    deleteFileInt
            jp c,   popDBH_ret
            jp      fileRename_noExisting
fileRename_missing:
            ld a,(v_dosError)
            cp DOS_ERR_NOT_FOUND
            jp nz,stc_popDBH_ret
fileRename_noExisting:

            ; Повторно найти исходный файл: предыдущий поиск мог вытеснить
            ; его сектор и сделать v_cachedDescrPtr недействительным.
            ex      de, hl                  ; hl = старое, de = новое
            call    fileFind
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
