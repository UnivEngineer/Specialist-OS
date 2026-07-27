;----------------------------------------------------------------------------
; MXOS NC.COM
; Копирование выбранного файла через временный 32-байтовый дескриптор.
;
; Файл пока целиком загружается в 0000h. Размер ограничен 36 КБ, чтобы не
; затереть экран и системную область. Потоковое копирование будет отдельным
; изменением DOS ABI.
;----------------------------------------------------------------------------

copyFileInt:
            ; Явно выбрать диск активной панели.
            ld      hl, state.panelA_drive
            call    readBytePanel
            ld      e, 1
            call    bios_fileGetSetDrive

            ; Скопировать имя выбранного файла в полный дескриптор.
            call    getSelectedFile
            ld      de, vars.tempFileDescr
            ld      c, FILE_NAME_LENGTH + 3
copyFileNameLoop:
            ld      a, (hl)
            ld      (de), a
            inc     hl
            inc     de
            dec     c
            jp nz,  copyFileNameLoop

            ; Получить полный дескриптор до загрузки и проверить размер.
            ld      hl, vars.tempFileDescr
            call    bios_fileLoadInfo
            jp c,   copyFileReadError

            ld      hl, vars.tempFileDescr + FILE_DESCRIPTOR.size + 2
            ld      a, (hl)
            inc     hl
            or      (hl)
            jp nz,  copyFileTooLarge
            ld      hl, (vars.tempFileDescr + FILE_DESCRIPTOR.size)
            ld      a, h
            cp      090h
            jp nc,  copyFileTooLarge

            ld      hl, (vars.tempFileDescr + FILE_DESCRIPTOR.loadAddress)
            ld      (vars.copyLoadAddr), hl

            ; Загрузить исходный файл в буфер 0000h.
            ld      hl, vars.tempFileDescr
            ld      de, 0
            call    bios_fileLoad2
            jp c,   copyFileReadError

            ; Подготовить имя назначения. fileNamePrepare одновременно
            ; выбирает диск из префикса X:.
            ld      hl, vars.input
            ld      de, vars.tempFileDescr
            call    bios_fileNamePrepare

            ; Не передавать невалидный v_diskInfo файловым функциям.
            ld      b, 3
            call    bios_diskDriver
            ld      a, d
            or      e
            jp z,   copyFileNoDriver
            ld      bc, DISK_INFO.isValid
            add     hl, bc
            ld      a, (hl)
            cp      DISK_VALID
            jp nz,  copyFileNotFormatted

            ; fileCreate берёт данные по loadAddress, поэтому источник 0000h.
            xor     a
            ld      (vars.tempFileDescr + FILE_DESCRIPTOR.loadAddress), a
            ld      (vars.tempFileDescr + FILE_DESCRIPTOR.loadAddress + 1), a

            ld      hl, vars.tempFileDescr
            call    bios_fileCreate
            jp c,   copyFileCreateError

            ; Вернуть файлу исходный адрес загрузки.
            ld      hl, (vars.copyLoadAddr)
            ex      de, hl
            ld      hl, vars.tempFileDescr
            ld      c, 1
            call    bios_fileGetSetAddr
            ret nc

            ld      hl, aCantSetFileAddr
            jp      copyFileShowError

copyFileReadError:
            ld      hl, aCantReadFile
            jp      copyFileShowError

copyFileNoDriver:
            ld      hl, aDestNoDriver
            jp      copyFileShowError

copyFileNotFormatted:
            ld      hl, aDestNotFormatted
            jp      copyFileShowError

copyFileCreateError:
            and     a
            ld      hl, aRootDirFull
            jp z,   copyFileShowError
            ld      hl, aDiskIsFull
            jp      copyFileShowError

copyFileTooLarge:
            ld      hl, aFileTooLarge

copyFileShowError:
            push    hl
            call    drawWindow2
            COLOR   COLOR_DIALOG_ERR
            CENTER_LINE 20, 80h
            pop     hl
            call    bios_printString

            COLOR   COLOR_DIALOG
            ld      hl, 5090h
            ld      (bios_vars.cursorY), hl
            ld      hl, vars.input
            call    printStringInv

copyFileAnyKey:
            call    bios_keyScan
            inc     a
            jp z,   copyFileAnyKey
            jp      mainReload