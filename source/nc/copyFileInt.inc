;----------------------------------------------------------------------------
; MXOS NC.COM
; Потоковое копирование через ОЗУ пользователя 0000h-7FFFh.
;----------------------------------------------------------------------------

COPY_BUFFER         = 0000h
COPY_BUFFER_SECTORS = 128

copyFileInt:
            ; Выбрать диск-источник и запомнить выбранную запись каталога.
            ld      hl, state.panelA_drive
            call    readBytePanel
            ld      (vars.copySourceDrive), a
            ld      e, 1
            call    bios_fileGetSetDrive

            call    getSelectedFile
            ld      (vars.copySourceName), hl

            ; Загрузить полный дескриптор источника во временную запись.
            ld      de, vars.tempFileDescr
            ld      c, FILE_NAME_LENGTH + 3
copyFileNameLoop:
            ld      a, (hl)
            ld      (de), a
            inc     hl
            inc     de
            dec     c
            jp nz,  copyFileNameLoop

            ld      hl, vars.tempFileDescr
            call    bios_fileLoadInfo
            jp c,   copyFileError

            ld      hl, (vars.tempFileDescr + FILE_DESCRIPTOR.firstCluster)
            ld      (vars.copySourceCluster), hl

            ; Размер хранится как size-1. Число секторов должно помещаться
            ; в 16-битное адресное пространство текущей DOS.
            ld      hl, vars.tempFileDescr + FILE_DESCRIPTOR.size + 1
            ld      e, (hl)
            inc     hl
            ld      d, (hl)
            inc     hl
            ld      a, (hl)
            or      a
            jp nz,  copyFileError
            inc     de
            ld      a, d
            or      e
            jp z,   copyFileError
            ex      de, hl
            ld      (vars.copyBlocksLeft), hl

            ; Подготовить имя назначения. Явный префикс X: также выбирает
            ; соответствующий диск.
            ld      hl, vars.input
            ld      de, vars.tempFileDescr
            call    bios_fileNamePrepare

            xor     a
            ld      e, a
            call    bios_fileGetSetDrive
            ld      (vars.copyDestDrive), a

            ; Не позволять fileCreatePrepare удалить сам источник.
            ld      hl, vars.copySourceDrive
            cp      (hl)
            jp nz,  copyFileCheckDest
            ld      hl, (vars.copySourceName)
            ld      de, vars.tempFileDescr
            ld      c, FILE_NAME_LENGTH + 3
            call    bios_stricmp
            jp z,   copyFileError

copyFileCheckDest:
            ; Введённый префикс может указывать на диск без драйвера.
            ld      b, 3
            call    bios_diskDriver
            ld      a, d
            or      e
            jp z,   copyFileError
            ld      de, DISK_INFO.isValid
            add     hl, de
            ld      a, (hl)
            cp      DISK_VALID
            jp nz,  copyFileError

            ; Создать запись каталога и выделить полную цепочку FAT.
            xor     a
            ld      (bios_vars.diskWriteErr), a
            ld      hl, (vars.copyBlocksLeft)
            ex      de, hl
            ld      hl, vars.tempFileDescr
            call    bios_fileCreatePrepare
            jp c,   copyFileCleanup
            ld      a, (bios_vars.diskWriteErr)
            or      a
            jp nz,  copyFileCleanup

            ld      hl, (vars.tempFileDescr + FILE_DESCRIPTOR.firstCluster)
            ld      (vars.copyDestCluster), hl

copyFileLoop:
            ; Один проход переносит до 32 КБ: в обычном случае диск
            ; переключается один раз для чтения и один раз для записи.
            ld      hl, (vars.copyBlocksLeft)
            ld      c, COPY_BUFFER_SECTORS
            ld      a, h
            or      a
            jp nz,  copyFileBatchReady
            ld      a, l
            cp      COPY_BUFFER_SECTORS + 1
            jp nc,  copyFileBatchReady
            ld      c, a

copyFileBatchReady:
            ld      a, (vars.copySourceDrive)
            ld      e, 1
            call    bios_fileGetSetDrive
            ld      hl, (vars.copySourceCluster)
            ex      de, hl
            ld      hl, COPY_BUFFER
            ld      b, 2
            call    bios_fileTransfer
            jp c,   copyFileCleanup
            ex      de, hl
            ld      (vars.copySourceCluster), hl

            ld      a, (vars.copyDestDrive)
            ld      e, 1
            call    bios_fileGetSetDrive
            xor     a
            ld      (bios_vars.diskWriteErr), a
            ld      hl, (vars.copyDestCluster)
            ex      de, hl
            ld      hl, COPY_BUFFER
            ld      b, 1
            call    bios_fileTransfer
            jp c,   copyFileCleanup
            ex      de, hl
            ld      (vars.copyDestCluster), hl
            ld      a, (bios_vars.diskWriteErr)
            or      a
            jp nz,  copyFileCleanup

            ld      hl, (vars.copyBlocksLeft)
            ld      d, 0
            ld      e, c
            call    bios_sub_hl_de
            ld      (vars.copyBlocksLeft), hl
            ld      a, h
            or      l
            jp nz,  copyFileLoop
            ret

copyFileCleanup:
            ld      a, (vars.copyDestDrive)
            ld      e, 1
            call    bios_fileGetSetDrive
            ld      hl, vars.tempFileDescr
            call    bios_fileDelete

copyFileError:
            ld      hl, aCopyFailed

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

copyFileWaitRelease:
            call    bios_keyScan
            inc     a
            jp nz,  copyFileWaitRelease

copyFileAnyKey:
            call    bios_keyScan
            inc     a
            jp z,   copyFileAnyKey
            jp      mainReload