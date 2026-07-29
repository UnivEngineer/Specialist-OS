;----------------------------------------------------------------------------
; MXOS TAPE.COM
; Текстовый интерфейс загрузки и сохранения файлов на магнитофон.
;
; Файл временно размещается по адресу 0000h. Максимальный размер ограничен
; 36 КБ, чтобы не затереть экран по адресу 9000h.
;----------------------------------------------------------------------------

TAPE_BUFFER_END = 08FFFh

Start:
tapeMenu:
            ; После магнитофонной операции экран и состояние курсора могут
            ; остаться произвольными. Меню всегда начинает с чистого экрана.
            ld      hl, 0
            ld      (bios_vars.inverse), hl
            call    bios_clearScreenOld
            ld      hl, 0008h
            ld      (bios_vars.cursorY), hl
            ld      a, 0FFh
            ld      (bios_vars.escMode), a

            ; Этот стек восстанавливается при ошибке чтения или по Break.
            ld      hl, 0
            add     hl, sp
            ld      (v_savedSP), hl

            ld      hl, tapeOperationAbort
            ld      (bios_vars.tapeError), hl

            ld      hl, txtMenu
            call    bios_printString

tapeMenuWait:
            call    bios_getch
            cp      1Bh
            jp z,   tapeExit
            call    bios_upperCase
            cp      'L'
            jp z,   tapeLoadFile
            cp      'S'
            jp z,   tapeSaveFile
            jp      tapeMenuWait

tapeExit:
            jp      bios_reboot

;----------------------------------------------------------------------------
; Загрузка файла с магнитофона
;----------------------------------------------------------------------------

tapeLoadFile:
            call    chooseDestinationDrive
            jp c,   tapeMenu

            ld      hl, txtWaiting
            call    bios_printString
            call    tapePrepareHardware
            call    clearFileDescriptor

            ; Найти маркер D9h, D9h, D9h после первого пилот-тона.
tapeFindHeader:
            ld      a, 0FFh
            call    tapeRead
            cp      0D9h
            jp nz,  tapeFindHeader
            ld      b, 2
tapeFindHeaderNext:
            ld      a, 8
            call    tapeRead
            cp      0D9h
            jp nz,  tapeFindHeader
            dec     b
            jp nz,  tapeFindHeaderNext

            ; Имя и расширение: 11 символов FAT без точки.
            ld      hl, v_fileDescr
            ld      b, FILE_NAME_LENGTH + 3
tapeReadName:
            ld      a, 8
            call    tapeRead
            cp      ' '
            jp nc,  tapeReadNameStore
            ld      a, ' '
tapeReadNameStore:
            ld      (hl), a
            inc     hl
            dec     b
            jp nz,  tapeReadName

            ; Второй пилот-тон, начальный и конечный адреса.
            ld      a, 0FFh
            call    tapeReadWordA
            ld      (v_tapeLoadAddr), hl
            ex      de, hl                  ; de = начальный адрес
            ld      a, 8
            call    tapeReadWordA           ; hl = конечный адрес

            call    bios_cmp_hl_de
            jp c,   tapeBadRange
            call    bios_sub_hl_de          ; hl = размер файла - 1
            ld      a, h
            cp      (TAPE_BUFFER_END + 1) >> 8
            jp nc,  tapeTooLarge

            ld      (v_fileSize), hl
            ld      (v_fileDescr + FILE_DESCRIPTOR.size), hl
            xor     a
            ld      (v_fileDescr + FILE_DESCRIPTOR.size + 2), a
            ld      (v_fileDescr + FILE_DESCRIPTOR.size + 3), a
            ld      (v_fileDescr + FILE_DESCRIPTOR.loadAddress), a
            ld      (v_fileDescr + FILE_DESCRIPTOR.loadAddress + 1), a

            ; Прочитать size+1 байт в нижнюю память.
            inc     hl
            ex      de, hl
            ld      hl, 0
tapeReadData:
            ld      a, 8
            call    tapeRead
            ld      (hl), a
            inc     hl
            dec     de
            ld      a, d
            or      e
            jp nz,  tapeReadData

            ld      a, 8
            call    tapeReadWordA
            ld      (v_tapeCRC), hl

            ld      hl, (v_fileSize)
            ex      de, hl
            ld      hl, 0
            call    bios_calcCS
            ld      hl, (v_tapeCRC)
            ld      a, b
            cp      h
            jp nz,  tapeBadCRC
            ld      a, c
            cp      l
            jp nz,  tapeBadCRC

            ; Создать файл из буфера 0000h и восстановить исходный адрес
            ; загрузки, записанный на ленте.
            ld      hl, v_fileDescr
            call    bios_fileCreate
            jp c,   tapeCreateError

            ld      hl, (v_tapeLoadAddr)
            ex      de, hl
            ld      hl, v_fileDescr
            ld      c, 1
            call    bios_fileGetSetAddr
            jp c,   tapeCreateError

            ld      hl, txtLoadComplete
            jp      tapeMessageAndMenu

;----------------------------------------------------------------------------
; Сохранение файла на магнитофон
;----------------------------------------------------------------------------

tapeSaveFile:
            call    clearFileDescriptor
            ld      hl, txtFilePrompt
            call    bios_printString
            ld      hl, v_path
            ld      de, v_path + 21
            call    bios_input

            ld      hl, v_path
            ld      de, v_fileDescr
            call    bios_fileNamePrepare

            ld      hl, v_fileDescr
            call    bios_fileLoadInfo
            jp c,   tapeFileNotFound

            ; Текущая реализация DOS хранит размер как size-1.
            ld      hl, v_fileDescr + FILE_DESCRIPTOR.size
            ld      e, (hl)
            inc     hl
            ld      d, (hl)
            inc     hl
            ld      a, (hl)
            inc     hl
            or      (hl)
            jp nz,  tapeTooLarge
            ld      a, d
            cp      (TAPE_BUFFER_END + 1) >> 8
            jp nc,  tapeTooLarge
            ex      de, hl
            ld      (v_fileSize), hl

            ld      hl, (v_fileDescr + FILE_DESCRIPTOR.loadAddress)
            ld      (v_tapeLoadAddr), hl

            ; Загрузить файл в буфер 0000h.
            ld      hl, v_fileDescr
            ld      de, 0
            call    bios_fileLoad2
            jp c,   tapeFileNotFound

            ld      hl, (v_fileSize)
            ex      de, hl
            ld      hl, 0
            call    bios_calcCS
            ld      h, b
            ld      l, c
            ld      (v_tapeCRC), hl

            ld      hl, txtSaving
            call    bios_printString
            call    tapePrepareHardware

            call    tapeWritePilot
            ld      a, 0D9h
            call    tapeWrite
            call    tapeWrite
            call    tapeWrite

            ld      hl, v_fileDescr
            ld      bc, FILE_NAME_LENGTH + 2
            call    tapeWriteBlock

            call    tapeWritePilot

            ld      hl, (v_tapeLoadAddr)
            call    tapeWriteWord
            push    hl
            ld      hl, (v_fileSize)
            ex      de, hl
            pop     hl
            add     hl, de
            call    tapeWriteWord

            ld      hl, (v_fileSize)
            ld      b, h
            ld      c, l
            ld      hl, 0
            call    tapeWriteBlock

            ld      hl, (v_tapeCRC)
            call    tapeWriteWord

            ld      hl, txtSaveComplete
            jp      tapeMessageAndMenu

;----------------------------------------------------------------------------
; Общие подпрограммы интерфейса
;----------------------------------------------------------------------------

; Выход:
;   CF=0 — выбран существующий драйвер, диск уже активен;
;   CF=1 — пользователь нажал Esc.
; Разрушает: AF, BC, DE, HL.
chooseDestinationDrive:
chooseDriveAgain:
            ld      hl, txtDrivePrompt
            call    bios_printString
            call    bios_getch
            cp      1Bh
            jp z,   chooseDriveCancel
            call    bios_upperCase
            cp      'A'
            jp c,   chooseDriveInvalid
            cp      'I'
            jp nc,  chooseDriveInvalid
            push    af
            ld      c, a
            call    bios_printChar
            pop     af
            sub     'A'
            ld      e, 1
            call    bios_fileGetSetDrive

            ld      b, 3
            call    bios_diskDriver
            ld      a, d
            or      e
            jp z,   chooseDriveNoDriver
            or      a
            ret

chooseDriveInvalid:
            ld      hl, txtInvalidDrive
            call    bios_printString
            jp      chooseDriveAgain

chooseDriveNoDriver:
            ld      hl, txtNoDriver
            call    bios_printString
            jp      chooseDriveAgain

chooseDriveCancel:
            scf
            ret

clearFileDescriptor:
            ld      hl, v_fileDescr
            ld      b, FILE_DESCR_SIZE
            xor     a
clearFileDescriptorLoop:
            ld      (hl), a
            inc     hl
            dec     b
            jp nz,  clearFileDescriptorLoop
            ret

tapePrepareHardware:
            ld      a, 0Fh
            ld      (IO_KEYB_C), a
            ld      a, 0FEh
            ld      (IO_KEYB_A), a
            ret

; Вход: A — режим чтения первого байта слова (0FFh или 8).
; Выход: HL — слово в little endian.
; Разрушает: AF.
tapeReadWordA:
            call    tapeRead
            ld      l, a
            ld      a, 8
            call    tapeRead
            ld      h, a
            ret

; Записывает bc+1 байт от hl.
tapeWriteBlock:
            inc     bc
tapeWriteBlockLoop:
            call    tapeCheckBreak
            ld      a, (hl)
            call    tapeWrite
            inc     hl
            dec     bc
            ld      a, b
            or      c
            jp nz,  tapeWriteBlockLoop
            ret

; 200h нулевых байт и маркер E6h.
tapeWritePilot:
            ld      bc, 0200h
tapeWritePilotLoop:
            call    tapeCheckBreak
            xor     a
            call    tapeWrite
            dec     bc
            ld      a, b
            or      c
            jp nz,  tapeWritePilotLoop
            ld      a, 0E6h
            jp      tapeWrite

tapeCheckBreak:
            ld      a, (IO_KEYB_B)
            and     a
            jp p,   tapeOperationAbort
            ret

tapeOperationAbort:
            ld      hl, (v_savedSP)
            ld      sp, hl
            ld      hl, txtAborted
            jp      tapeMessageAndMenu

tapeBadRange:
            ld      hl, txtBadRange
            jp      tapeMessageAndMenu

tapeTooLarge:
            ld      hl, txtTooLarge
            jp      tapeMessageAndMenu

tapeBadCRC:
            ld      hl, txtBadCRC
            jp      tapeMessageAndMenu

tapeCreateError:
            ld      hl, txtCreateError
            jp      tapeMessageAndMenu

tapeFileNotFound:
            ld      hl, txtFileNotFound

tapeMessageAndMenu:
            call    bios_printString
            ld      hl, txtPressKey
            call    bios_printString
            call    bios_getch
            cp      1Bh
            jp z,   tapeExit
            jp      tapeMenu

;----------------------------------------------------------------------------
; Строки и переменные
;----------------------------------------------------------------------------

txtMenu:
            DB "TAPE UTILITY",0Ah
            DB "L - LOAD FROM TAPE",0Ah
            DB "S - SAVE FILE TO TAPE",0Ah
            DB "ESC - EXIT TO NC",0Ah,0
txtDrivePrompt:
            DB 0Ah,"DESTINATION DRIVE [A-H]: ",0
txtInvalidDrive:
            DB 0Ah,"INVALID DRIVE",0Ah,0
txtNoDriver:
            DB 0Ah,"DRIVE HAS NO DRIVER",0Ah,0
txtWaiting:
            DB 0Ah,"WAITING FOR TAPE. PRESS A KEY TO ABORT...",0
txtFilePrompt:
            DB 0Ah,"FILE: ",0
txtSaving:
            DB 0Ah,"SAVING. PRESS A KEY TO ABORT...",0
txtLoadComplete:
            DB 0Ah,"LOAD COMPLETE",0
txtSaveComplete:
            DB 0Ah,"SAVE COMPLETE",0
txtBadRange:
            DB 0Ah,"INVALID ADDRESS RANGE",0
txtTooLarge:
            DB 0Ah,"FILE IS LARGER THAN 36 KB",0
txtBadCRC:
            DB 0Ah,"CRC ERROR",0
txtCreateError:
            DB 0Ah,"CAN'T CREATE FILE",0
txtFileNotFound:
            DB 0Ah,"FILE NOT FOUND",0
txtAborted:
            DB 0Ah,"ABORTED",0
txtPressKey:
            DB 0Ah,"ANY KEY - MENU, ESC - NC",0

v_savedSP:
            DW 0
v_tapeLoadAddr:
            DW 0
v_fileSize:
            DW 0
v_tapeCRC:
            DW 0
v_path:
            BLOCK 22, 0
v_fileDescr:
            FILE_DESCRIPTOR
