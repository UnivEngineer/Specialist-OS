; Файловые клавиши и вход с аргументом командной строки
; Порядок включения задан главным .asm; см. README.md.

;----------------------------------------------------------------------

        ; Обработка кода нажатой клавиши

keyInput:
        CALL editorGetch
        JP C,editorControlKey
        CP 8
        JP C,editorFunctionKey
        JP keyInput2
editorWriteDocument:
        CALL  inputFileName
        LD A,(editorFileCancelled)
        OR A
        RET NZ
        CALL  saveFile
        LD A,(hexActive)
        OR A
        RET NZ
        JP    synchronizeTopAndRedraw

;----------------------------------------------------------------------

        ; Сохранённый байт выравнивания исходной версии.
        NOP

prepareFilePrompt:
        LD    HL,FILE_NAME_HISTORY
        RET
; Проверка до загрузки: точный N и завершающий маркер должны войти в буфер.
; Выход: CF=1 — файл не помещается между v_textBeginLo и v_textLimitLo.
; Разрушает AF, DE, HL, флаги; BC сохраняет.
; Учитывает верхнее слово размера, но не суммарный размер добавления F6.
editorCheckFileSize:
    IF !STANDARD_EDITOR
        LD    HL,(fileDescSize + 2)
        LD    A,H
        OR    L
        JP    NZ,editorCheckFileSizeError
    ENDIF
        LD    HL,(v_textBeginLo)
        EX    DE,HL
        LD    HL,(v_textLimitLo)
        CALL  bios_sub_hl_de
        EX    DE,HL
        LD    HL,(fileDescSize)
        CALL  bios_cmp_hl_de
        JP    NC,editorCheckFileSizeError
        OR    A
        RET

editorCheckFileSizeError:
        SCF
        RET

editorShowFileTooLarge:
        LD HL,editorFileTooLargeMessage
        JP editorErrorModal

;----------------------------------------------------------------------------
; MXOS
;
; Инициализировать документ и обработать стартовый путь.
;
; вход:
;   de = аргументы ASCIIZ. Префикс -NC подавляет баннер в MAIN; графический вызов EXEC
;   передаёт путь напрямую и использует ebQuiet.
;
; выход:
;   editorStart получает загруженный или пустой документ. MAIN регистрирует буфер в DOS
;   до первой записи. При нехватке памяти, недопустимом размере или пути к каталогу
;   показан диалог и вызван bios_reboot.
;----------------------------------------------------------------------------

openCommandLineFile:
        ; NC already drew its launch notice. A banked graphical caller
        ; supplies a plain path; the COM handoff prefixes it with -NC.
    IF EDIT_BANKED
        LD A,(ebQuiet)
        OR A
        JP NZ,editorLoadingDone
    ENDIF
        PUSH DE
        EX DE,HL
        CALL bios_argNext
        JP C,editorConsoleBanner
        LD A,B
        CP 3
        JP NZ,editorConsoleBanner
        LD A,(DE)
        CP '-'
        JP NZ,editorConsoleBanner
        INC DE
        LD A,(DE)
        CP 'N'
        JP NZ,editorConsoleBanner
        INC DE
        LD A,(DE)
        CP 'C'
        JP NZ,editorConsoleBanner
        POP DE
        EX DE,HL
        JP editorLoadingDone
editorConsoleBanner:
        CALL editorLoadingBanner
        POP DE
editorLoadingDone:
        PUSH  DE
        CALL  bios_getMemTop
        LD    DE,EDITOR_MEMORY_END-1
        CALL  bios_cmp_hl_de
        JP    C,editorLimitReady
        EX    DE,HL
editorLimitReady:
        INC   HL
        LD    (v_textLimitLo),HL
        LD    DE,EDITOR_TEXT_BEGIN+2
        CALL  bios_cmp_hl_de
        POP   DE
        JP    C,editorMemoryTooSmall
    IF !EDIT_BANKED
        ; Register the whole gap/snapshot/document range before its first write.
        PUSH DE
        LD HL,(v_textBeginLo)
        EX DE,HL
        LD HL,(v_textLimitLo)
        CALL bios_sub_hl_de
        EX DE,HL
        LD HL,(v_textBeginLo)
        LD B,0
        LD A,MEM_OP_MAINREGISTER
        MXOS_CALL MXOS_DOS_MEMORY
        POP DE
        JP C,editorMemoryTooSmall
    ENDIF
        ; Новый документ начинается пустым, даже если в памяти осталась NC.
        LD    HL,(v_textBeginLo)
        LD    (v_textEnd),HL
        LD    (HL),00h
        XOR   A
        call bios_setFontEncoding      ; Файлы MXOS хранятся в KOI-8.
        EX    DE,HL
        CALL  bios_argNext
        JP    C,editorStart
        EX    DE,HL
        LD    DE,fileDescName
        PUSH  HL
        MXOS_CALL MXOS_DOS_FILE_NAME_PREPARE
        POP   HL
        JP    C,editorStart
        LD    DE,FILE_NAME_HISTORY
        LD    B,MAX_PATH-1
editorRememberPath:
        LD    A,(HL)
        CP    021h
        JP    C,editorPathEnd
        LD    (DE),A
        INC   HL
        INC   DE
        DEC   B
        JP    NZ,editorRememberPath
editorPathEnd:
        XOR   A
        LD    (DE),A
        LD    HL,fileDescName
        MXOS_CALL MXOS_DOS_FILE_LOAD_INFO
        JP    C,editorCheckStartupDirectory
        CALL  editorCheckFileSize
        JP    C,commandLineFileTooLarge
        LD    HL,(v_textBeginLo)
        EX    DE,HL
        LD    HL,fileDescName
        CALL  loadTextFile
        CALL  NC,editorDetectHex
        JP    editorStart

commandLineFileTooLarge:
        CALL  editorShowFileTooLarge
        JP    bios_reboot

editorCheckStartupDirectory:
        LD HL,fileDescName
        LD BC,0
        MXOS_CALL MXOS_DOS_FILE_GET_SET_ATTR
        JP C,editorStart
        AND 10h
        JP Z,editorStart
        LD HL,editorDirectoryMessage
        LD DE,editorOpenErrorTitle
        CALL editorErrorWithTitle
        JP bios_reboot

        DB    00H,00H

; Reject before even writing the initial NUL when no text space remains.
editorMemoryTooSmall:
        LD HL,editorMemoryMessage
        CALL editorErrorModal
        JP bios_reboot
editorMemoryMessage:        DB 'No memory for text buffer.',0
editorFileTooLargeMessage:  DB 'File does not fit in memory.',0
editorDirectoryMessage:     DB 'Path is a directory.',0

editorControlKey:
        CP 0Ch
        JP Z,keyHandler_DocumentHome
        CP 0Ah
        JP Z,keyHandler_DocumentEnd
        CP 19h
        JP Z,editorPageUp
        CP 1Ah
        JP Z,editorPageDown
        CP 8
        JP Z,movePreviousWord
        CP 18h
        JP Z,moveNextWord
        AND 7Fh
        AND 0DFh
        CP 'Z'
        JP Z,editorUndoLine
        RET
