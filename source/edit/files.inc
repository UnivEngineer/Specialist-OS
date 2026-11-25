; Дисковые операции, ввод имени и дескриптор файла
; Порядок включения задан главным .asm; см. README.md.

; Вход: прежнее имя в FILE_NAME_HISTORY. Выход: inputBuf с именем; Enter принимает его.
; Esc уходит в цикл редактора. Разрушает AF, BC, DE, HL, флаги.
; Буфер и история имеют MAX_PATH байт, включая завершающий нуль.
inputFileName:
        LD HL,txtFile
        JP inputFilePrompt
inputAppendName:
        LD HL,txtAppendFile
inputFilePrompt:
        XOR A
        LD (editorFileCancelled),A
        LD (editorFilePrompt),HL
        CALL prepareFilePrompt
        LD DE,FILE_NAME_RECALL_END
        LD BC,inputBuf
        CALL bios_memcpy_bc_hl
        CALL editorOpenFileModal
        LD HL,inputBuf
        LD D,H
        LD E,L
editorFileEnd:
        LD A,(DE)
        OR A
        JP Z,editorFileInput
        INC DE
        JP editorFileEnd
editorFileInput:
        LD A,(editorInputWidth)
        LD C,A
        LD B,11h               ; Escape returns, first printable replaces recall
        CALL lineEdit
        PUSH AF
        CALL editorCloseModal
        POP AF
        CP 13
        RET Z
        JP cancelFilePrompt
finishFilePrompt: LD    HL,inputBuf
        LD    DE,inputBuf+MAX_PATH
        LD    BC,FILE_NAME_HISTORY
        CALL  bios_memcpy_bc_hl
        LD    HL,0000H
        LD    (bios_vars.inverse),HL
        RET

;----------------------------------------------------------------------
; Загрузка файла с диска

openFile:
        CALL  prepareFileName
        JP    C,filePathError
        LD    HL,fileDescName
        MXOS_CALL_REGS MXOS_DOS_FILE_LOAD_INFO
        JP    C,fileOperationError
        CALL  editorCheckFileSize
        JP    C,editorFileTooLarge
        LD    HL,(v_textBeginLo)
        EX    DE,HL
        LD    HL,fileDescName
        CALL  editorLoadWithProgress
        JP    C,fileOperationError
        XOR A
        LD (v_documentDirty),A
        CALL finishFilePrompt
        JP editorDetectHex

editorFileTooLarge:
        CALL editorShowFileTooLarge
        JP editorCloseModal

;----------------------------------------------------------------------
; Подготовка имени файла для файловых операций

prepareFileName:
        LD    HL,inputBuf
        LD    DE,fileDescName
        MXOS_CALL_REGS MXOS_DOS_FILE_NAME_PREPARE
        RET

;----------------------------------------------------------------------
; Сохранение файла на диск

saveFile:
        LD HL,(v_textBeginLo)
        EX DE,HL
        LD HL,(v_textEnd)
        CALL bios_sub_hl_de
        LD (fileDescSize),HL
        LD HL,editorBusySaving
        CALL editorShowFileProgress
        CALL editorOpenSaveFile
        JP C,editorSavePathError
        CALL editorWriteGapFile
        CALL editorSaveProgressDone
        JP C,editorSaveError
        XOR A
        LD (v_documentDirty),A
        JP finishFilePrompt

editorSaveProgressDone:
        PUSH AF
        CALL editorFinishFileProgress
        POP AF
        RET
editorOpenSaveFile:
        CALL flClear
        LD HL,inputBuf
        LD (flHandle+FH_BUFFER),HL
        LD A,FILE_MODE_WRITE|FILE_MODE_CREATE|FILE_MODE_TRUNCATE
        LD (flHandle+FH_MODE),A
        LD A,FILE_OP_OPEN_PATH
        JP flCall

; Write the two physical spans, excluding the gap and the undo snapshot.
editorWriteGapFile:
        LD HL,(v_textBeginLo)
        LD (flHandle+FH_BUFFER),HL
        EX DE,HL
        LD HL,(v_gapLo)
        CALL bios_sub_hl_de
        LD (flHandle+FH_COUNT),HL
        LD A,FILE_OP_WRITE
        CALL flCall
        JP C,flClose
        LD HL,(v_gapHi)
        LD (flHandle+FH_BUFFER),HL
        LD HL,(v_gapLo)
        EX DE,HL
        LD HL,(v_textEnd)
        CALL bios_sub_hl_de
        LD (flHandle+FH_COUNT),HL
        LD A,FILE_OP_WRITE
        CALL flCall
        JP flClose

editorSavePathError:
        CALL editorSaveProgressDone
        LD DE,editorSaveErrorTitle
        JP filePathErrorText
editorSaveError:
        LD DE,editorSaveErrorTitle
        JP fileOperationErrorText
filePathError:
        LD DE,editorOpenErrorTitle
filePathErrorText:
        CP DOS_ERR_ARGUMENT
        JP NZ,fileOperationErrorText
        LD HL,editorInvalidPath
        JP fileOperationShowError
fileOperationError:
        LD DE,editorOpenErrorTitle
fileOperationErrorText:
        CALL dosErrorText
fileOperationShowError:
        CALL editorErrorWithTitle
        JP editorCloseModal
editorSaveErrorTitle:   DB 'Cannot save file',0
editorOpenErrorTitle:   DB 'Cannot open file',0
editorInvalidPath:      DB 'Invalid file name or path',0
;----------------------------------------------------------------------

; Вход: inputBuf. Выход: файл добавлен по логическому концу либо сигнал ошибки.
; Разрушает AF, BC, DE, HL, флаги.
; Размер проверяется до чтения; исключительная граница оставляет место для NUL.
insertFile:
        CALL  prepareFileName
        JP    C,filePathError
        LD    HL,fileDescName

        ; Запрашиваем дескриптор файла
        MXOS_CALL_REGS MXOS_DOS_FILE_LOAD_INFO
        JP    C,fileOperationError
        CALL  editorCheckFileSize
        JP    C,editorFileTooLarge
        LD    HL,(fileDescSize)
        EX    DE,HL         ; de = размер файла
        LD    HL,(v_textEnd)
        PUSH  HL    ; hl = адрес конца буфера
        ADD   HL,DE
        JP    C,editorAppendTooLarge
        EX    DE,HL
        LD    HL,(v_textLimitLo)
        LD    A,H
        CP    D
        JP    C,editorAppendTooLarge
        JP    NZ,loadAppendFile
        LD    A,L
        CP    E
        JP    C,editorAppendTooLarge
        JP    Z,editorAppendTooLarge
        JP loadAppendFile

        ; загрузка файла по адресу de
editorAppendTooLarge:
        POP HL
        JP editorFileTooLarge
loadAppendFile: POP   DE
        LD    HL,fileDescName
        CALL  editorLoadWithProgress
        JP    C,fileOperationError
        LD A,1
        LD (v_documentDirty),A
        JP    finishFilePrompt

;----------------------------------------------------------------------

editorLoadWithProgress:
        PUSH HL
        LD HL,editorBusyLoading
        CALL editorShowFileProgress
        POP HL
        CALL loadTextFile
        JP C,editorLoadProgressError
        ; The complete redraw replaces the notice after a successful read.
        LD A,(editorProgressVisible)
        OR A
        RET Z
        XOR A
        LD (editorProgressVisible),A
    IF EDIT_BANKED
        JP ebDiscardModal
    ELSE
        RET
    ENDIF
editorLoadProgressError:
        PUSH AF
        PUSH BC
        PUSH DE
        CALL editorFinishFileProgress
        POP DE
        POP BC
        POP AF
        RET

; HL=дескриптор, DE=буфер. Нулевой файл тоже получает маркер конца текста.
; Сохраняет BC/DE, разрушает AF/HL; CF=ошибка чтения.
loadTextFile:
        CALL  fileReadDescriptor
        RET   C
        LD    HL,(fileDescSize)
        ADD   HL,DE
        LD    (HL),00h
        LD    (v_textEnd),HL
        OR    A
        RET

cancelFilePrompt:
        LD A,1
        LD (editorFileCancelled),A
        RET

;----------------------------------------------------------------------

txtFile:                DB  "File name",0
txtAppendFile:          DB  "Append file",0
editorFilePrompt:       DW  txtFile
editorFileCancelled:    DB  0

    IF STANDARD_EDITOR

inputBuf:       BLOCK 10, 00h
inputBufEnd:    DB    00H,00H,00H

; Дескриптор файла (запись в каталоге MXOS FAT8)
fileDescName:   BLOCK 10, 20h   ; имя, расширение и атрибуты
fileDescDate:   DW    0         ; дата доступа, не адрес
fileDescSize:   DW    2020H     ; размер
fileDescCRC:    DB    20H       ; контрольная сумма
                DB    20H       ; первый сектор

    ELSE

inputBuf:       BLOCK MAX_PATH-1, 00h
inputBufEnd:    DB    00H

; Дескриптор файла (запись в каталоге FAT16)
fileDescName:   BLOCK 12, 20h   ; имя, расширение и атрибуты
                BLOCK 6, 0
fileDescDate:   DW    0         ; дата доступа, не адрес
                BLOCK 8, 0
fileDescSize:   DW    2020H     ; размер
                BLOCK 2, 0

    ENDIF

txtSpace12:     DB   "            ",00H

