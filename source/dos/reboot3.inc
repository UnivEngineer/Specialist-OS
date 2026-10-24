;+---------------------------------------------------------------------------
; MXOS
; Обновление драйвера по адресу 0FFC0h и запуск NC.COM,
; а также однократный запуск AUTOEXEC.BAT, если не нажата клавиша.
;
; 2013-12-12 Дизассемблировано vinxru
;----------------------------------------------------------------------------

reboot3:    ; Инициализация стека
            ld      sp, STACK_ADDR

            ; Приложение могло изменить таймер и параметры BIOS-звука.
            call    bios_reset_sound

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

            ; Запуск файла A:AUTOEXEC.BAT
            ld      hl, pathAutoexecBat
            call    fileExec

; -------------------------------------

reboot3_2:
            ; Запуск файла A:NC.COM
            ld      hl, pathNcCom
            call    fileExec

            ; Теплая перезагрузка
            jp      j_reboot3


; Общий сброс звука при холодной и горячей перезагрузке.
; Входа нет. A/HL разрушаются; BC/DE и флаги сохраняются.
; 27 байт, 122 такта (139 с CALL); четыре записи аппаратных регистров,
; два чтения шаблона и две записи переменных BIOS. Новых переменных нет.
; Режим 3 без загрузки счёта останавливает каждый канал ВИ53, OUT=1.
; BSR-команда 0Ah выключает программный звук PC5, не меняя режим ВВ55.
; Параметры берутся из общего шаблона, а не из состояния чужой программы.
bios_reset_sound:
            ld      a, 0B6h
            ld      (IO_TIMER_MODE), a
            ld      a, 076h
            ld      (IO_TIMER_MODE), a
            ld      a, 036h
            ld      (IO_TIMER_MODE), a
            ld      a, 00Ah
            ld      (IO_KEYB_MODE), a
            ld      hl, (initVars.beepFreq)
            ld      (bios_vars.beepFreq), hl
            ret

            ASSERT bios_vars.beepDuration == bios_vars.beepFreq+1
