; Дисковые операции, ввод имени и дескриптор файла
; Порядок включения задан главным .asm; см. README.md.

; Вход: прежнее имя в FILE_NAME_HISTORY. Выход: inputBuf с именем; Enter принимает его.
; СТР уходит в цикл редактора. Разрушает AF, BC, DE, HL, флаги.
; Восстановление имени копирует 10 байт, сохранение после ввода — 16 байт.
inputFileName:
        CALL  prepareFilePrompt
        LD    DE,FILE_NAME_RECALL_END
        LD    BC,inputBuf
        CALL  bios_memcpy_bc_hl
        LD    HL,00F9H
        LD    (bios_vars.cursorY),HL
        LD    HL,0FFFFH
        LD    (bios_vars.inverse),HL
        LD    HL,txtFile
        CALL  bios_printString
        CALL  bios_getch
        LD    HL,0FF9H
        LD    (bios_vars.cursorY),HL
        CP    0DH           ; 13
        JP    Z,finishFilePrompt
        PUSH  AF
        LD    HL,txtSpace12
        CALL  bios_printString
        LD    HL,0FF9H
        LD    (bios_vars.cursorY),HL
        LD    HL,inputBuf
        LD    BC,inputBuf
        LD    DE,inputBufEnd    ; 56781
        POP   AF
        JP    fileNameInputKey

finishFilePrompt: LD    HL,inputBuf
        LD    DE,inputBufEnd
        LD    BC,FILE_NAME_HISTORY
        CALL  bios_memcpy_bc_hl
        LD    HL,0000H
        LD    (bios_vars.inverse),HL
        RET

;----------------------------------------------------------------------
; Загрузка файла с диска

openFile:
        CALL  prepareFileName
        LD    HL,fileDescName
        CALL  bios_fileLoadInfo
        JP    C,fileOperationError
        CALL  editorCheckFileSize
        JP    C,editorFileTooLarge
        LD    HL,(v_textBeginLo)
        EX    DE,HL
        LD    HL,fileDescName
        CALL  bios_fileLoad2
        JP    C,fileOperationError
        JP    finishFilePrompt

editorFileTooLarge:
        CALL  editorShowFileTooLarge
        JP    finishFilePrompt

;----------------------------------------------------------------------
; Подготовка имени файла для файловых операций

prepareFileName:
        LD    HL,inputBuf
        LD    DE,fileDescName
        CALL  bios_fileNamePrepare
        RET

;----------------------------------------------------------------------
; Сохранение файла на диск

saveFile:
        CALL  prepareFileName
        LD    DE,0000H
        LD    B,00H
        LD    HL,(v_textBeginLo)

getCRC: ; Поиск конца буфера и подсчет контрольной суммы
    IF STANDARD_EDITOR
        ; В MXOS 2 контрольная сумма не нужна
        LD    A,B
        ADD   A,(HL)
        LD    B,A
    ENDIF
        INC   DE
        LD    A,(HL)
        INC   A
        JP    Z,buildSaveDescriptor
        INC   HL
        JP    getCRC

        ; Формирование дескриптора файла
buildSaveDescriptor: LD    HL,(v_textBeginLo)
        LD    (fileDescAddr),HL
        EX    DE,HL
        LD    (fileDescSize),HL

    IF STANDARD_EDITOR
        ; В MXOS 2 контрольная сумма не нужна
        LD    A,B
        LD    (fileDescCRC),A
    ENDIF

        ; Сохранение файла
        LD    HL,fileDescName
        CALL  bios_fileCreate
        JP    C,fileOperationError
        JP    finishFilePrompt

;----------------------------------------------------------------------

fileNameOverflow: POP   AF
        JP    synchronizeTopAndRedraw
        LD    B,H
        LD    C,L
fileNameInputLoop: CALL  bios_getch          ; [4]
fileNameInputKey: CP    08H           ; 8
        JP    Z,fileNameBackspace
        CP    0DH           ; 13
        JP    Z,terminateFileName
        CP    1FH           ; 31
        JP    Z,cancelFilePrompt
        CP    20H           ; 32 ' '
        JP    C,fileNameInputLoop
        PUSH  AF
        CALL  bios_cmp_hl_de
        JP    Z,fileNameOverflow
        POP   AF
        LD    (HL),A
        PUSH  BC
        LD    C,A
        CALL  bios_printChar
        POP   BC
        INC   HL
        JP    fileNameInputLoop
fileNameBackspace: LD    A,H
        CP    B
        JP    NZ,eraseFileNameCharacter
        LD    A,L
        CP    C
        JP    Z,fileNameInputLoop
eraseFileNameCharacter: DEC   HL
        PUSH  BC
        LD    C,08H         ; 8
        CALL  bios_printChar
        LD    C,20H         ; 32 ' '
        CALL  bios_printChar
        LD    C,08H         ; 8
        CALL  bios_printChar
        POP   BC
        JP    fileNameInputLoop
terminateFileName: LD    (HL),00H
        JP    finishFilePrompt
fileOperationError:  LD    C,05H         ; 5; [5]
fileErrorBeepLoop: CALL  bios_beep_Old
        LD    B,0FFH                ; 255
        CALL  bios_delay_b
        CALL  bios_delay_b
        DEC   C
        JP    NZ,fileErrorBeepLoop
        JP    finishFilePrompt
;----------------------------------------------------------------------

; Вход: inputBuf. Выход: файл добавлен поверх старого 0FFh либо сигнал ошибки.
; Разрушает AF, BC, DE, HL, флаги.
; ОШИБКА: после PUSH HL ветви нехватки места не снимают сохранённый указатель.
; Проверка также не учитывает полносекторную запись fileLoad2.
insertFile:
        CALL  prepareFileName
        LD    HL,fileDescName

        ; Запрашиваем дескриптор файла
        CALL  bios_fileLoadInfo
        JP    C,fileOperationError
        CALL  editorCheckFileSize
        JP    C,editorFileTooLarge
        LD    HL,(fileDescSize)
        EX    DE,HL         ; de = размер файла
        PUSH  DE
        LD    HL,(v_textBeginLo)    ; hl = адрес буфера

        ; Сканируем буфер, пока не найдем конец текста (0FFh)
findAppendPosition: LD    A,(HL)
        INC   A
        JP    Z,checkAppendCapacity
        INC   HL
        JP    findAppendPosition

checkAppendCapacity: POP   DE    ; de = размер файла
        PUSH  HL    ; hl = адрес конца буфера
        ADD   HL,DE
        EX    DE,HL
        LD    HL,(v_textLimitLo)
        LD    A,H
        CP    D
        JP    C,fileOperationError
        JP    NZ,loadAppendFile
        LD    A,L
        CP    E
        JP    C,fileOperationError

        ; загрузка файла по адресу de
loadAppendFile: POP   DE
        LD    HL,fileDescName
        CALL  bios_fileLoad2
        JP    C,fileOperationError
        JP    finishFilePrompt

;----------------------------------------------------------------------

cancelFilePrompt: POP   DE
        LD    HL,0000H
        LD    (bios_vars.inverse),HL
        JP    clearAndRedrawDocument

;----------------------------------------------------------------------

txtFile:        DB    "FILE:"

    IF STANDARD_EDITOR

inputBuf:       BLOCK 10, 00h
inputBufEnd:    DB    00H,00H,00H

; Дескриптор файла (запись в каталоге MXOS FAT8)
fileDescName:   BLOCK 10, 20h   ; имя, расширение и атрибуты
fileDescAddr:   DW    2020H     ; адрес загрузки
fileDescSize:   DW    2020H     ; размер
fileDescCRC:    DB    20H       ; контрольная сумма
                DB    20H       ; первый сектор

    ELSE

inputBuf:       BLOCK 16, 00h
inputBufEnd:    DB    00H,00H,00H

; Дескриптор файла (запись в каталоге FAT16)
fileDescName:   BLOCK 12, 20h   ; имя, расширение и атрибуты
                BLOCK 6, 0
fileDescAddr:   DW    2020H     ; адрес загрузки
                BLOCK 8, 0
fileDescSize:   DW    2020H     ; размер
                BLOCK 2, 0

    ENDIF

txtSpace12:     DB   "            ",00H

