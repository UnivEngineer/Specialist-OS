;----------------------------------------------------------------------------
; MXOS NC.COM
; Получить информацию о выбранном файле (FILE_INFO)
;
; выход:
;   hl - адрес структуры FILE_INFO
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

        ; Адрес описателя текущего файла в буфере:
        ; hl = видимое окно + FILE_INFO_SIZE * относительный индекс;

        ld      hl, FILE_INFO_SIZE
        ld      e, a
        ld      d, 0
        call    bios_mul_hl_de

        ex      de, hl
        ld      hl, (vars.fileListPtr)
        add     hl, de

        pop     de
        ret

