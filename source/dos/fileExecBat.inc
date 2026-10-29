;+---------------------------------------------------------------------------
; MXOS
; Запустить BAT файл. Вызывается функцией fileExec
;
; 2013-12-12 Дизассемблировано vinxru
;----------------------------------------------------------------------------

execBat:
            pop     de

            ; Загружаем BAT во временный буфер без изменения записи каталога.
            ld      hl, BAT_BUFFER
            ld      (v_batPtr), hl

            ; Сохраняем имя BAT-файла
            ld      hl, v_curFileDescr      
            ld      de, v_curFileDescr + FILE_NAME_LENGTH + 3
            ld      bc, v_batFileDescr
            call    memmove_bc_hl

            ; Сохраняем диск содержащий BAT-файл
            ld      a, (v_drive)
            ld      (v_batDrive+1), a   ; самомодификация кода

            ; Этот цикл выполняется для каждой строки BAT-файла
execBat_loop:

            ; Сохраняем текущий диск в регистре b
            ld      a, (v_drive)
            ld      b, a

v_batDrive:
            ; Выбрать диск содержащий BAT файл
            ld      a, 1 ; << сюда вместо 1 пишется диск, содержащий BAT файл
            call    fileSelectDrive

            ; Проверяем размер до загрузки, оставляя место для терминатора.
            ld      hl, v_batFileDescr
            call    fileLoadInfo
            jp c,   badCommand
            ld      a, (v_batFileDescr + FILE_DESCRIPTOR.size + 3)
            or      a
            jp nz,  badCommand
            ld      a, (v_batFileDescr + FILE_DESCRIPTOR.size + 2)
            or      a
            jp nz,  badCommand
            ld      a, (v_batFileDescr + FILE_DESCRIPTOR.size + 1)
            cp      BAT_BUFFER_SIZE / 256 ; максимум 32 КБ
            jp nc,  badCommand

            ; Загружаем BAT и завершаем его даже на границе сектора.
            ld      hl, v_batFileDescr
            ld      de, BAT_BUFFER
            call    fileLoad2
            jp c,   badCommand
            ld      hl, (v_batFileDescr + FILE_DESCRIPTOR.size)
            inc     hl              ; размер в каталоге хранится как N-1
            ld      de, BAT_BUFFER
            add     hl, de
            ld      (hl), 0FFh
            ld      hl, (v_batPtr)

            ; Сохраняем начало строки
            ld      d, h
            ld      e, l

            ; Ищем конец строки
execBat_0:
            ld      a, (hl)       
            inc     hl

            ; Найден конец строки, запускаем файл
            cp      0Dh         
            jp z,   execBat_1

            ; Принимать также Unix-перевод строки. Файлы из Git и
            ; современных редакторов часто заканчиваются байтом 0Ah.
            cp      0Ah
            jp z,   execBat_1

            ; Конец файла; непустую последнюю строку выполняем без CR/LF.
            cp      0FFh
            jp nz,  execBat_0
            dec     hl
            ld      a, h
            cp      d
            jp nz,  execBat_lastLine
            ld      a, l
            cp      e
            ret z
execBat_lastLine:
            ld      (hl), 0Dh
            jp      execBat_1

; ---------------------------------------------------------------------------

execBat_1:
            ; Сохраняем указатель
            ld      (v_batPtr), hl  

            ; Восстановление активного диска          
            ld      a, b         
            call    fileSelectDrive

            ; Пустые строки и второй байт CR/LF не являются командами.
            ld      a, (de)
            cp      0Dh
            jp z,   execBat_loop
            cp      0Ah
            jp z,   execBat_loop

            ; Запустить файл
            ex      de, hl             
            call    fileExec
            ret c                       ; ошибка прерывает пакетный файл

            jp      execBat_loop

