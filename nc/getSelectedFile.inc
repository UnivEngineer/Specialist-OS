;----------------------------------------------------------------------------
; MXOS NC.COM
; Получить имя выбранного файла
;
; выход:
;   hl - адрес дескриптора файла
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

getSelectedFile:
        push    de

        ; Получить номер текущего файла
        ld      hl, panelA_curFile
        call    readBytePanel

        ; Получить имя (адрес) текущего файла
        ; вычисляется hl = FILE_LIST_BUFFER + DIR_DESCR_SIZE * curFile[activePanel];
        ld      hl, FILE_LIST_BUFFER - DIR_DESCR_SIZE
        ld      de, DIR_DESCR_SIZE
loc_D80F:
        add     hl, de
        dec     a
        jp p,   loc_D80F

        pop     de
        ret

