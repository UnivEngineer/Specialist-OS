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

        ; С какого файла начинается список файлов панели
        ld      hl, state.panelA_firstFile
        call    readBytePanel
        ld      e, a

        ; Номер текущего файла
        ld      hl, state.panelA_curFile
        call    readBytePanel
        sub     e

        ; Адрес дескриптора текущего файла в буфере:
        ; hl = FILE_LIST_BUFFER + DIR_DESCR_SIZE * curFile[state.activePanel];

        ld      hl, DIR_DESCR_SIZE
        ld      e, a
        ld      d, 0
        call    bios_mul_hl_de

        ld      de, FILE_LIST_BUFFER
        add     hl, de

        pop     de
        ret

