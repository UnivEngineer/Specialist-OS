;+---------------------------------------------------------------------------
; MXOS
; Обновление драйвера по адресу 0FFC0h и запуск NC.COM,
; а также однократный запуск AUTOEXEC.BAT, если не нажата клавиша.
;
; 2013-12-12 Дизассемблировано vinxru
;----------------------------------------------------------------------------

dosBoot:    ; Инициализация стека; BIOS уже загрузил DOS
            ld      sp, STACK_ADDR

            ; Приложение могло изменить таймер и параметры BIOS-звука.
            call    bios_reset_sound

            ; Восстанавливаем обычный режим доступа к RAM-диску
            call    nativeInstallBlock

            ; Обновляем переменные
            ld      hl, j_reboot3
            ld      (mon2_vars.tapeError), hl

reboot3_0:
            jp      reboot3_1

; -------------------------------------
; Код ниже будет выполнен лишь один раз после холодной перезагрузки.
; То есть при теплой перезагрузке он не вызывается.

reboot3_1:
            ; Патчим предыдущую инструкцию - теперь она стала jp reboot3_2
            ld      hl, reboot3_2
            ld      (reboot3_0+1), hl

            ; Только линковочный native-профиль проверяет историческую метку.
            ld      hl, nativeMarkImage
            ld      de, nativeMarkEnd
            ld      bc, 0FFC0h
            call    memcpy_bc_hl
            ld      hl, 0FFBFh
            call    0FFC0h
            push    af
            call    nativeInstallBlock
            pop     af
            jp z,   skipFormat

            ; Запуск файла "A:FORMAT.BAT"
            ; До форматирования B: нельзя восстанавливать его контекст
            ; после поиска ассоциаций. Сначала выбрать системный том.
            ld a,SYSTEM_DRIVE
            call fileSelectDrive
            ld      hl, pathFormatBat
            call    fileExec

skipFormat:
            ; Если нажата клавиша, пропустить запуск A:AUTOEX.BAT
            call    ioKeyReadRows
            and     2
            jp z,   reboot3_2

            ; Драйверы выбираются только явными командами AUTOEXEC.
            ld a,SYSTEM_DRIVE
            call    fileSelectDrive

reboot3_autoexec:
            ; Запуск файла A:AUTOEXEC.BAT
            ld      a, 1
            ld      (v_bootAutoexec), a
            ld      hl, pathAutoexecBat
            call    fileExec
            ld      a, 0
            ld      (v_bootAutoexec), a ; LD сохраняет CF после fileExec
            jp nc,  reboot3_2

reboot3_fallback:
            ; Отменяем установленный AUTOEXEC драйвер A: и заново монтируем том.
            ld      hl, diskDriver
            ld      (v_drives+2*SYSTEM_DRIVE), hl
            ld      (j_diskDriver+1), hl
            ld a,SYSTEM_DRIVE
            ld      (v_drive), a
            call    mountVolume

; -------------------------------------

reboot3_2:
            ; Запуск файла A:NC.COM
            ld a,(v_shell)
            or a
            ld hl,pathNcCom
            jp z,rebootShellReady
            ld hl,pathCliCom
rebootShellReady:
            call fileExec

            ; Теплая перезагрузка
            jp      j_reboot3

;----------------------------------------------------------------------------
; MXOS
; Проверить и установить метку форматирования в нулевом банке native-DOS.
; вход: код исполняется из общего окна, HL указывает на историческую метку.
; выход: MAIN, Z при прежней метке 5Ah, AF изменены, BC/DE/HL/SP сохранены.
;----------------------------------------------------------------------------
nativeMarkImage:
            xor     a
            ld      (IO_PAGE_RAMD), a
            ld      a, (hl)
            ld      (hl), 5Ah
            ld      (IO_PAGE_RAM), a
            cp      5Ah
            ret
nativeMarkEnd:
            ASSERT nativeMarkEnd-nativeMarkImage <= 32

; Установить секторное окно линковочного профиля DOS.native.
nativeInstallBlock:
            ld      hl, driverBlockStart
            ld      de, driverBlockEnd
            ld      bc, NATIVE_RAMD_READ
            jp      memcpy_bc_hl

; Встроенный блочный драйвер целиком помещается в общее окно.
; FFC0h/FFD0h: C=страница, DE=сектор<<8 (E=0), HL=буфер основной RAM.
; Передаёт ровно 256 байт. C сохраняется; AF/B/DE/HL разрушаются.
; При выходе снова включена основная RAM; стек не переключается.
driverBlockStart:
            ld      a,c
            ld      (IO_PAGE_RAMD),a
            ld      a,(de)
            ld      (IO_PAGE_RAM),a
            ld      (hl),a
            inc     hl
            inc     e
            jp nz,  NATIVE_RAMD_READ
            ret
            nop
driverBlockWrite:
            ld      b,(hl)
            ld      a,c
            ld      (IO_PAGE_RAMD),a
            ld      a,b
            ld      (de),a
            ld      (IO_PAGE_RAM),a
            inc     hl
            inc     e
            jp nz,  NATIVE_RAMD_WRITE
            ret
driverBlockEnd:
    ASSERT driverBlockWrite-driverBlockStart == NATIVE_RAMD_WRITE-NATIVE_RAMD_READ
    ASSERT driverBlockEnd-driverBlockStart == 32
