;----------------------------------------------------------------------------
; MXOS NC.COM
; Загрузить список файлов с накопителя
;
; вход:
;   a = номер накопителя
;
; 2013-12-18 Дизассемблировано vinxru
; 2022-02-16 Доработано SpaceEngineer
;----------------------------------------------------------------------------

loadFiles:  ; Получение спика файлов в буфере
           
            ld      e, 1
            call    bios_fileGetSetDrive  ; устанавливем активные накопитель

            ; bc = файл, с которого начинается спосок файлов панели
            ld      hl, state.panelA_firstFile
            call    readBytePanel
            ld      c, a
            ld      b, 0    ; считаем, что файлов в каталоге меньше 256

            ; Получаем список файлов в буфере и общее
            ; количество файлов в каталоге в de
            ld      hl, FILE_LIST_BUFFER  ; адрес буфера
            ld      de, FILE_LIST_SIZE    ; размер буфера
            call    bios_fileList

            ; Сохранение количествава файлов
            ld      a, e                ; считаем, что файлов в каталоге меньше 256
            ld      hl, state.panelA_filesCnt
            call    writeBytePanel

            ; После удаления последнего файла старый индекс может оказаться
            ; за концом списка. Для пустой панели обе позиции равны нулю.
            ld      c, a
            or      a
            jp z,   lf_StoreCursor
            dec     a
            ld      b, a                ; последний допустимый индекс
            ld      hl, state.panelA_curFile
            call    readBytePanel
            cp      c
            ret c
            ld      a, b
lf_StoreCursor:
            ld      hl, state.panelA_curFile
            call    writeBytePanel
            ld      c, a
            ld      hl, state.panelA_firstFile
            call    readBytePanel
            cp      c
            ret c
            ret z

            ; Начало видимой части тоже вышло за конец: сдвигаем его
            ; к выбранному файлу и заново загружаем буфер списка.
            ld      a, c
            call    writeBytePanel
            ld      hl, state.panelA_drive
            call    readBytePanel
            jp      loadFiles

