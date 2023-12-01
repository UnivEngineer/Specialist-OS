;+---------------------------------------------------------------------------
; MXOS
; Запустить BAT файл. Вызывается функцией fileExec
;
; 2013-12-12 Дизассемблировано vinxru
;----------------------------------------------------------------------------

execBat:
            pop     de

            ; Устанавливаем адрес загрузки BAT-файла на BAT_BUFFER (= FC00h)
            ld      hl, BAT_BUFFER
            ld      (v_batPtr), hl
            ex      de, hl

            ; Изменяем адрес загрузки BAT-файла
            ; TODO: это не работает с ром-диском (A: без поддержки записи). Поэтому
            ; нельзя запустить BAT-файл с диска A:, если его адрес загрузки в каталоге
            ; уже не прошит как FC00h (= BAT_BUFFER)
            ld      hl, v_curFileDescr
            ld      c, 1
            call    fileGetSetAddr
            jp c,   badCommand

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

            ; Загрузить BAT-файл в память
            ld      hl, v_batFileDescr 
            call    fileLoad
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

            ; Конец файла
            cp      0FFh        
            jp nz,  execBat_0
            ret

; ---------------------------------------------------------------------------

execBat_1:
            ; Сохраняем указатель
            ld      (v_batPtr), hl  

            ; Восстановление активного диска          
            ld      a, b         
            call    fileSelectDrive

            ; Запустить файл
            ex      de, hl             
            call    fileExec

            jp      execBat_loop

