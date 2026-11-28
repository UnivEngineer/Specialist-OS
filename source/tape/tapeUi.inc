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
            xor     a
            call FONT_ENGINE_ENCODING    ; Имена файлов — KOI-8.
            ; После магнитофонной операции экран и состояние курсора могут
            ; остаться произвольными. Меню всегда начинает с чистого экрана.
            ld      hl, 0
            ld      (mon2_vars.inverse), hl
            call    mon2_clearScreenOld
            ld      hl, 0008h
            ld      (mon2_vars.cursorY), hl
            ld      a, 0FFh
            ld      (bios_upper.escMode), a

            ; Этот стек восстанавливается при ошибке чтения или по Break.
            ld      hl, 0
            add     hl, sp
            ld      (v_savedSP), hl

            ld      hl, tapeOperationAbort
            ld      (mon2_vars.tapeError), hl

            ld      hl, txtMenu
            call    mon2_printStringOld

tapeMenuWait:
            call    mon2_getchOld
            cp      1Bh
            jp z,   tapeExit
            MXOS_CALL_REGS MXOS_BIOS_UPPERCASE
            cp      'L'
            jp z,   tapeLoadFile
            cp      'S'
            jp z,   tapeSaveFile
            jp      tapeMenuWait

tapeExit:
            jp      mon2_init

;----------------------------------------------------------------------------
; Загрузка файла с магнитофона
;----------------------------------------------------------------------------

tapeLoadFile:
            call    chooseDestinationDrive
            jp c,   tapeMenu

            ld      hl, txtWaiting
            call    mon2_printStringOld
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

            call    mon2_cmp_hl_de
            jp c,   tapeBadRange
            MXOS_CALL_REGS MXOS_BIOS_SUB_HL_DE          ; hl = размер файла - 1
            ld      a, h
            cp      (TAPE_BUFFER_END + 1) >> 8
            jp nc,  tapeTooLarge

            ld      (v_fileSize), hl
            ld de,7
            add hl,de
            ld      (v_fileDescr + FILE_DESCRIPTOR.size), hl
            xor     a
            ld      (v_fileDescr + FILE_DESCRIPTOR.size + 2), a
            ld      (v_fileDescr + FILE_DESCRIPTOR.size + 3), a

            ; Прочитать size+1 байт в нижнюю память.
            ld hl,(v_fileSize)
            inc hl
            ex de,hl
            ld hl,4
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

            ld hl,(v_fileSize)
            ld de,4
            add hl,de
            ex de,hl
            ld hl,4
            call bios_calcCS
            ld      hl, (v_tapeCRC)
            ld      a, b
            cp      h
            jp nz,  tapeBadCRC
            ld      a, c
            cp      l
            jp nz,  tapeBadCRC

            ; Сохранить стандартный RKS вместе с адресами и суммой.
            ld hl,(v_tapeLoadAddr)
            ld (0),hl
            push hl
            ld hl,(v_fileSize)
            ex de,hl
            pop hl
            add hl,de
            ld (2),hl
            ld hl,(v_fileSize)
            ld de,5
            add hl,de
            push hl
            ld hl,(v_tapeCRC)
            ex de,hl
            pop hl
            ld (hl),e
            inc hl
            ld (hl),d
            ld hl,v_fileDescr+8
            ld (hl),'R'
            inc hl
            ld (hl),'K'
            inc hl
            ld (hl),'S'
            ld hl,v_fileDescr
            ld de,0
            call fileWriteDescriptor
            jp c,tapeCreateError

            ld      hl, txtLoadComplete
            jp      tapeMessageAndMenu

;----------------------------------------------------------------------------
; Сохранение файла на магнитофон
;----------------------------------------------------------------------------

tapeSaveFile:
            call    clearFileDescriptor
            ld      hl, txtFilePrompt
            call    mon2_printStringOld
            ld      hl, v_path
            ld      de, v_path + 21
            MXOS_CALL_REGS MXOS_BIOS_INPUT

            ld      hl, v_path
            ld      de, v_fileDescr
            MXOS_CALL_REGS MXOS_DOS_FILE_NAME_PREPARE
            ld      hl, v_fileDescr
            MXOS_CALL_REGS MXOS_DOS_FILE_LOAD_INFO
            jp c,   tapeFileNotFound

            ; В каталоге хранится точная длина контейнера RKS.
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

            ld      hl,0
            ld      (v_tapeLoadAddr), hl

            ; Загрузить файл в буфер 0000h.
            ld      hl, v_fileDescr
            ld      de, 0
            call    fileReadDescriptor
            jp c,   tapeFileNotFound

            ld hl,(0)
            ld (v_tapeLoadAddr),hl
            ex de,hl
            ld hl,(2)
            MXOS_CALL_REGS MXOS_BIOS_SUB_HL_DE
            jp c,tapeBadCRC
            ld (v_fileSize),hl
            ld de,7
            add hl,de
            push hl
            ld hl,(v_fileDescr+FILE_DESCRIPTOR.size)
            ex de,hl
            pop hl
            call mon2_cmp_hl_de
            jp nz,tapeBadCRC
            ld hl,(v_fileSize)
            ld de,5
            add hl,de
            ld b,(hl)
            inc hl
            ld c,(hl)
            ld h,c
            ld l,b
            ld (v_tapeCRC),hl
            ld hl,(v_fileSize)
            ld de,5
            add hl,de
            ex de,hl
            ld hl,4
            ld bc,0
            call mon2_memcpy_bc_hl
            ld      hl, (v_fileSize)
            ex      de, hl
            ld      hl, 0
            call    bios_calcCS
            ld      hl,(v_tapeCRC)
            ld      a,h
            cp      b
            jp nz,  tapeBadCRC
            ld      a,l
            cp      c
            jp nz,  tapeBadCRC

            ld      hl, txtSaving
            call    mon2_printStringOld
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
            call    mon2_printStringOld
            call    mon2_getchOld
            cp      1Bh
            jp z,   chooseDriveCancel
            MXOS_CALL_REGS MXOS_BIOS_UPPERCASE
            cp      'A'
            jp c,   chooseDriveInvalid
            cp      'I'
            jp nc,  chooseDriveInvalid
            push    af
            ld      c, a
            call    mon2_printCharOld
            pop     af
            sub     'A'
            ld      e, 1
            MXOS_CALL_REGS MXOS_DOS_FILE_GET_SET_DRIVE
            ld      b, 3
            MXOS_CALL_REGS MXOS_BIOS_DISK_DRIVER
            ld      a, d
            or      e
            jp z,   chooseDriveNoDriver
            or      a
            ret

chooseDriveInvalid:
            ld      hl, txtInvalidDrive
            call    mon2_printStringOld
            jp      chooseDriveAgain

chooseDriveNoDriver:
            ld      hl, txtNoDriver
            call    mon2_printStringOld
            jp      chooseDriveAgain

chooseDriveCancel:
            scf
            ret

;----------------------------------------------------------------------------
; MXOS
;
; Обнулить рабочий файловый дескриптор магнитофонной утилиты.
;
; вход:
;   v_fileDescr = собственный буфер на FILE_DESCR_SIZE байтов.
;
; выход:
;   Все байты равны нулю, hl указывает сразу за буфером. a = b = 0, z = 1, cf = 0. c и
;   de сохранены.
;----------------------------------------------------------------------------

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

;----------------------------------------------------------------------------
; MXOS
;
; Установить исходные значения портов перед обменом с магнитофоном.
;
; вход:
;   Параметров нет. Доступны регистры ВВ55 IO_KEYB_C и IO_KEYB_A.
;
; выход:
;   В IO_KEYB_C записано 0Fh, в IO_KEYB_A — 0FEh. a = 0FEh. bc, de, hl и флаги
;   сохранены.
;----------------------------------------------------------------------------

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

;----------------------------------------------------------------------------
; MXOS
;
; Записать диапазон байтов на ленту с проверкой отмены перед каждым байтом.
;
; вход:
;   hl = начало данных, bc = длина минус единица. Магнитофонный порт и v_savedSP
;   подготовлены.
;
; выход:
;   При обычном возврате hl продвинут за блок, bc = 0, a = 0, cf = 0 и z = 1. При
;   отмене управление переходит в меню с восстановлением v_savedSP.
;----------------------------------------------------------------------------

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

;----------------------------------------------------------------------------
; MXOS
;
; Записать пилот из 512 нулевых байтов и синхробайт E6h.
;
; вход:
;   Выход на магнитофон настроен, v_savedSP сохранён для отмены операции.
;
; выход:
;   При завершении bc = 0, a = E6h. При отмене во время пилота управление передаётся в
;   меню вместо возврата.
;----------------------------------------------------------------------------

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

;----------------------------------------------------------------------------
; MXOS
;
; Проверить клавишу прерывания текущей магнитофонной операции.
;
; вход:
;   Бит 7 порта IO_KEYB_B активен низким уровнем. v_savedSP указывает на стек
;   вызывающего меню.
;
; выход:
;   При отпущенной клавише возвращается a = прочитанный порт, s = 1 и cf = 0. При
;   нажатии восстанавливается sp и выводится сообщение об отмене, возврата к операции
;   нет.
;----------------------------------------------------------------------------

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
            call    mon2_printStringOld
            ld      hl, txtPressKey
            call    mon2_printStringOld
            call    mon2_getchOld
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
