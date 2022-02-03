;+---------------------------------------------------------------------------
; MXOS
; Обновление драйвера по адресу 0FFC0h и запуск NC.COM
;
; А так же однократный запуск AUTOEX.BAT, если не нажата клавиша.
;
; 2013-12-12 Дизассемблировано vinxru
;----------------------------------------------------------------------------

reboot3:    ; Инициализация стека
            ld      sp, STACK_ADDR

            ; Устанавливаем блочный драйвер
            ld      b, 0
            call    setRAMDDriver

            ; Обновляем переменные
            ld      hl, j_reboot3
            ld      (bios_vars.tapeError), hl

reboot3_0:
            jp      reboot3_1

; -------------------------------------
; Код ниже будет выполнен лишь один раз после холодной перезагрузки.
; То есть при теплой перезагрузке он не вызывается.

reboot3_1:
            ; Патчим предыдущую инструкцию - теперь она стала jp reboot3_2
            ld      hl, reboot3_2
            ld      (reboot3_0+1), hl

            ; Устанавливаем побайтовый драйвер
            ld      b, 1
            call    setRAMDDriver

            ; Если в рам-диске по адресу 0FF00h (последний неполный сектор)
            ; нет специальной метки, значит, не запускали форматирование
            ld      hl, 0FFBFh      ; адрес в странице
            xor     a               ; 0 страница
            call    bios_RAMDRead
            ld      a, c            ; считанный байт в c
            cp      5Ah
            jp z,   skipFormat      ; метка есть - пропускаем форматирование

            ; Записываем метку
            xor     a               ; 0 страница
            ld      c, 5Ah          ; записываемый байт
            call    bios_RAMDWrite

            ; Устанавливаем блочный драйвер
            ld      b, 0
            call    setRAMDDriver

            ; Запуск файла "A:FORMAT.BAT"
            ld      hl, pathFormatBat
            call    fileExec

skipFormat:
            ; Если нажата клавиша, пропустить запуск A:AUTOEX.BAT
            ld      a, (IO_KEYB_B)     
            and     2
            jp z,   reboot3_2

            ; Запуск файла A:AUTOEX.BAT
            ld      hl, pathAutoexecBat
            call    fileExec

; -------------------------------------

reboot3_2:
            ; Запуск файла A:NC.COM
            ld      hl, pathNcCom
            call    fileExec

            ; Теплая перезагрузка
            jp      j_reboot3

