; Имя 8.3 используется для диагностики. Следующие десять байтов BAT_FILE
; соответствуют firstCluster, size и FH_POSITION в общем дескрипторе чтения.
BAT_DRIVE    = 11
BAT_FILE     = 12
BAT_RESUME   = BAT_FILE+6
BAT_LINE     = BAT_RESUME+4
BAT_OFFSET   = BAT_LINE+2
BAT_CR       = BAT_OFFSET+4
    ASSERT BAT_CR+1 == BAT_FRAME_SIZE
    ASSERT FILE_DESCRIPTOR.firstCluster+6 == FH_POSITION

;----------------------------------------------------------------------------
; MXOS
;
; Выполнить BAT из разрешённого каталога и возобновить вызывающий сценарий.
;
; вход:
;   v_curFileDescr = короткое имя, v_drive и v_directory задают каталог.
;   На стеке лежат указатель аргументов и адрес возврата загрузчика.
;
; выход:
;   cf = 0 при успехе, иначе cf = 1 и a = ошибка DOS. Кадр родителя
;   восстановлен. af, bc, de и hl используются.
;----------------------------------------------------------------------------
execBat:
    pop de
    ld a,(v_batDepth)
    cp BAT_MAX_DEPTH
    jp nc,blockArgument
    or a
    call nz,batDetach
    ld a,(v_batDepth)
    or a
    jp nz,batNested
    ld (v_batReported),a
batNested:
    inc a
    ld (v_batDepth),a
    ld hl,(v_batFrame)
    push hl
    ld de,BAT_FRAME_SIZE
    add hl,de
    ld (v_batFrame),hl
    ex de,hl
    ld b,BAT_FRAME_SIZE
    xor a
    call memset_de_a_b
    ld hl,(v_batFrame)
    ex de,hl
    ld hl,v_curFileDescr
    ld bc,11
    call cliCopyBytes
    ld a,(v_drive)
    ld (de),a
    ld hl,v_curFileDescr
    call fileFind
    jp nc,batFound
    ld a,(v_dosError)
    jp batDone
batFound:
    ld a,BAT_FILE
    call batField
    ex de,hl
    ld hl,v_tmpFileDescr+FILE_DESCRIPTOR.firstCluster
    ld bc,6
    call cliCopyBytes
    call batCheckCycle
    jp c,batUnwind          ; Ошибка относится к строке вызова родителя.
    call batRun
batDone:
    call c,batError
batUnwind:
    push af
    call batDiscard
    ld hl,v_batDepth
    dec (hl)
    pop bc
    pop hl
    ld (v_batFrame),hl
    push bc
    pop af
    ret

;----------------------------------------------------------------------------
; MXOS
;
; Отклонить повторный вход в сценарий, уже присутствующий в стеке.
;
; вход:
;   v_batFrame и v_batDepth описывают стек вместе с новым кадром.
;   v_tmpFileDescr содержит найденную запись, v_drive — диск файла.
;
; выход:
;   cf = 0, если можно продолжить. При совпадении диска и начального
;   кластера cf = 1 и a = DOS_ERR_ARGUMENT. Все регистры рабочие.
;----------------------------------------------------------------------------
batCheckCycle:
    ld a,(v_batDepth)
    dec a
    ld hl,(v_batFrame)
    ld de,0-BAT_FRAME_SIZE
    add hl,de
    call batFindActive
    ret nc
    jp blockArgument

;----------------------------------------------------------------------------
; MXOS
;
; Проверить, удерживает ли активный BAT цепочку найденного файла.
;
; вход:
;   a = число проверяемых кадров, hl = самый новый из них.
;   v_drive и v_tmpFileDescr задают том и первый кластер объекта.
;
; выход:
;   cf = 1 при совпадении, иначе cf = 0. af, bc, de, hl используются.
;----------------------------------------------------------------------------
batFindActive:
    or a
    ret z
    ld c,a
    ld de,BAT_DRIVE
    add hl,de
    ld a,(v_drive)
    ld b,a
    ex de,hl
    ld hl,(v_tmpFileDescr+FILE_DESCRIPTOR.firstCluster)
    ex de,hl
batActiveNext:
    ld a,b
    cp (hl)
    jp nz,batActiveSkip
    inc hl
    ld a,e
    cp (hl)
    inc hl
    jp nz,batActiveBack
    ld a,d
    cp (hl)
    jp z,batActiveFound
batActiveBack:
    dec hl
    dec hl
batActiveSkip:
    dec c
    jp z,batActiveAbsent
    push de
    ld de,0-BAT_FRAME_SIZE
    add hl,de
    pop de
    jp batActiveNext
batActiveAbsent:
    or a
    ret
batActiveFound:
    scf
    ret

;----------------------------------------------------------------------------
; MXOS
;
; Найти поле активного кадра BAT.
;
; вход:
;   a = байтовое смещение, v_batFrame = адрес кадра.
;
; выход:
;   hl указывает на поле, cf определяется сложением адресов. a, bc и de сохранены.
;----------------------------------------------------------------------------

batField:
    push de
    ld hl,(v_batFrame)
    ld e,a
    ld d,0
    add hl,de
    pop de
    ret

;----------------------------------------------------------------------------
; MXOS
;
; Прочитать порцию через общий FILE_HANDLE, сохранив каталог команды.
;
; вход:
;   BAT_HANDLE содержит открытый курсор чтения, адрес и размер запроса.
;
; выход:
;   cf/a = результат чтения либо ошибка восстановления диска. FH_COUNT =
;   число переданных байтов. bc, de и hl используются.
;----------------------------------------------------------------------------
batRead:
    ld a,(v_drive)
    push af
    ld hl,(v_directory)
    push hl
    ld a,FILE_OP_READ
    ld hl,BAT_HANDLE
    call fileHandle
    push af
    pop bc
    pop hl
    pop af
    push hl
    push bc
    call fileSelectDrive
    push af
    pop de
    pop bc
    pop hl
    ld (v_directory),hl
    push bc
    pop af
    ret c
    push de
    pop af
    ret

;----------------------------------------------------------------------------
; MXOS
;
; Вернуть общий дескриптор и буфер родителю или загрузчику DLL.
;
; вход:
;   v_batFrame указывает на активный сценарий. batLeft = непрочитанные байты.
;
; выход:
;   BAT_RESUME указывает на первый непрочитанный байт. batDiscard освобождает
;   окно без сохранения позиции при выходе из сценария. Все регистры рабочие.
;----------------------------------------------------------------------------
batDetach:
    ld a,(BAT_HANDLE+FH_STATE)
    or a
    ret z
    ld hl,(batLeft)
    ex de,hl
    ld a,BAT_RESUME
    call batSavePosition
batDiscard:
    xor a
    ld (BAT_HANDLE+FH_STATE),a
    ld hl,0
    ld (batWindowEnd),hl
    ld (batLeft),hl
    ret

;----------------------------------------------------------------------------
; MXOS
;
; Подготовить чтение целого BAT либо очередных 128 байтов потока.
;
; вход:
;   v_batFrame = активный сценарий, предыдущее окно исчерпано.
;
; выход:
;   cf/a = статус DOS, batPtr и batLeft описывают прочитанный диапазон.
;   Нулевой остаток при успехе означает EOF. Все регистры рабочие.
;----------------------------------------------------------------------------
batFill:
    ld a,(BAT_HANDLE+FH_STATE)
    or a
    jp nz,batCheckEOF
    ld a,BAT_DRIVE
    call batField
    ld a,(hl)
    ld (BAT_HANDLE+FH_DRIVE),a
    inc hl
    ld de,BAT_HANDLE+FILE_DESCRIPTOR.firstCluster
    ld bc,10
    call cliCopyBytes
    ld (BAT_HANDLE+FH_VALID),a   ; cliCopyBytes вернула ноль.
    ld a,FILE_MODE_READ
    ld (BAT_HANDLE+FH_MODE),a
    ld a,046h
    ld (BAT_HANDLE+FH_STATE),a
    IFDEF BANKED_DOS_BUILD
    ; Ищем место только при первом чтении. После вытеснения остаёмся в потоке.
    ld hl,BAT_HANDLE+FH_POSITION
    ld a,(hl)
    inc hl
    or (hl)
    inc hl
    or (hl)
    inc hl
    or (hl)
    jp nz,batSmall
    ld hl,(BAT_HANDLE+FILE_DESCRIPTOR.size+2)
    ld a,h
    or l
    jp nz,batSmall
    ld hl,(BAT_HANDLE+FILE_DESCRIPTOR.size)
    ld b,h
    ld c,l
    ld de,BAT_READ_SIZE
    call cmp_hl_de
    jp c,batSmall
    jp z,batSmall
    ld hl,DOS_BANK_MODULE_END
batFindHole:
    push hl
    ld d,b
    ld e,c
    call sub_hl_de
    pop de
    jp c,batSmall
    push de
    ld de,DOS_ARENA_START
    call cmp_hl_de
    pop de
    jp c,batSmall
    call arenaProbe
    jp nc,batWhole
    ld hl,(arenaObstacle)
    jp batFindHole
batWhole:
    ex de,hl
    ld (batWindowEnd),hl
    ex de,hl
    jp batReadWindow
    ELSE
    jp batSmall
    ENDIF
batCheckEOF:
    ; При пустом окне позиция ещё в общем дескрипторе. Конец уже известен:
    ; не выбираем дисковод повторно после последней команды на другом томе.
    ld hl,BAT_HANDLE+FH_POSITION
    ld de,BAT_HANDLE+FILE_DESCRIPTOR.size
    ld b,4
batCompareSize:
    ld a,(de)
    cp (hl)
    jp nz,batSmall
    inc de
    inc hl
    dec b
    jp nz,batCompareSize
    ld hl,0
    ret                     ; Последнее сравнение дало CF=0.
batSmall:
    ld hl,0
    ld (batWindowEnd),hl
    ld hl,BAT_INPUT
    ld bc,BAT_READ_SIZE
batReadWindow:
    ld (BAT_HANDLE+FH_BUFFER),hl
    ld (batPtr),hl
    ld h,b
    ld l,c
    ld (BAT_HANDLE+FH_COUNT),hl
    call batRead
    ret c
    ld hl,(BAT_HANDLE+FH_COUNT)
    ld (batLeft),hl
    ret

;----------------------------------------------------------------------------
; MXOS
;
; Получить следующий байт BAT из общего окна, пополняя его при необходимости.
;
; вход:
;   v_batFrame = текущий сценарий.
;
; выход:
;   cf = 0 и a = байт, cf = 1 и a = 0 при EOF, иначе cf = 1 и a = ошибка.
;   bc, de и hl сохранены.
;----------------------------------------------------------------------------
batByte:
    push hl
    ld hl,(batLeft)
    ld a,h
    or l
    jp nz,batByteReady
    push bc
    push de
    call batFill
    pop de
    pop bc
    jp c,batByteExit
    ld a,h
    or l
    scf
    jp z,batByteExit
batByteReady:
    dec hl
    ld (batLeft),hl
    ld hl,(batPtr)
    ld a,(hl)
    inc hl
    ld (batPtr),hl
    or a
batByteExit:
    pop hl
    ret

;----------------------------------------------------------------------------
; MXOS
;
; Разбирать строки текущего BAT и передавать их штатному FILE_EXEC.
;
; вход:
;   v_batFrame = активный кадр. Длина строки с нулём ограничена MAX_PATH.
;
; выход:
;   cf = 0 на конце файла, иначе a = ошибка чтения, синтаксиса или команды
;   и cf = 1. Кадр хранит номер строки и её смещение. Все регистры рабочие.
;----------------------------------------------------------------------------
batRun:
    call batByte
    jp c,batEnd
    push af
    ld a,BAT_CR
    call batField
    ld c,(hl)
    ld (hl),0
    pop af
    cp 0Ah
    jp nz,batFirst
    dec c
    jp z,batRun             ; LF после CR не создаёт вторую строку.
batFirst:
    push af
    ld a,BAT_LINE
    call batField
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc de
    ld a,d
    or e
    jp nz,batLineReady
    pop af
    jp blockArgument        ; Не допускать незаметного переполнения номера строки.
batLineReady:
    ld (hl),d
    dec hl
    ld (hl),e
    call batOffset
    ld hl,BAT_COMMAND
    ld b,MAX_PATH-1
    pop af
batCharacter:
    cp 0Dh
    jp z,batTerminator
    cp 0Ah
    jp z,batTerminator
    or a
    jp z,blockArgument      ; NUL внутри файла не является EOF.
    ld (hl),a
    inc hl
    dec b
    jp z,batTooLong
    call batByte
    jp nc,batCharacter
    or a
    jp nz,batFail
    xor a                   ; Последняя строка без CR/LF.
batTerminator:
    ld (hl),0
    push af
    ld a,BAT_CR
    call batField
    pop af
    cp 0Dh
    jp nz,batCommand
    ld (hl),1
batCommand:
    ld hl,BAT_COMMAND
    ld a,(hl)
    or a
    jp z,batRun
    cp '@'
    jp nz,batEcho
    inc hl
    jp batExecute
batEcho:
    ld a,(v_batEcho)
    or a
    jp z,batExecute
    push hl
    call j_printString
    call printCharA
    pop hl
batExecute:
    call fileExec
    ret c
    jp batRun
batTooLong:
    call batByte
    jp c,batLongEnd
    cp 0Dh
    jp z,batTerminator
    cp 0Ah
    jp z,batTerminator
    jp blockArgument
batLongEnd:
    or a
    jp nz,batFail
    jp batTerminator
batEnd:
    or a
    ret z
batFail:
    scf
    ret

;----------------------------------------------------------------------------
; MXOS
;
; Сохранить точную позицию строки или точку продолжения после вытеснения.
;
; вход:
;   batOffset учитывает уже выданный первый байт. Для batSavePosition
;   a = смещение поля кадра, de = число байтов для вычитания из FH_POSITION.
;
; выход:
;   поле кадра содержит 32-битное смещение от начала файла. b, de, hl и af
;   используются, c сохранён.
;----------------------------------------------------------------------------
batOffset:
    ld hl,(batLeft)
    ex de,hl
    inc de
    ld a,BAT_OFFSET
batSavePosition:
    call batField
    push hl
    push de
    ld de,BAT_HANDLE+FH_POSITION
    call handleCopy4
    pop de
    pop hl
    jp handleSubtractWord

batError:
    call dosSetError
    push af
    ld a,(v_batReported)
    or a
    jp nz,batErrorDone
    inc a
    ld (v_batReported),a
    ld hl,batErrorText
    call j_printString
    ld hl,(v_batFrame)
    ld b,8
    call batErrorName
    ld c,'.'
    call j_printChar
    ld b,3
    call batErrorName
    ld hl,batLineText
    call j_printString
    ld a,BAT_LINE
    call batField
    ld e,(hl)
    inc hl
    ld d,(hl)
    ld b,0
    call bios_printDecWord
    ld hl,batCodeText
    call j_printString
    pop af
    push af
    ld e,a
    ld d,0
    ld b,0
    call bios_printDecWord
    call printCharA
batErrorDone:
    ld a,(v_batDepth)
    dec a
    jp nz,batErrorReturn
    ld a,(v_bootAutoexec)
    or a
    jp nz,batErrorReturn
    ld a,(v_shell)
    or a
    jp nz,batErrorReturn
batWaitError:
    call j_getch            ; NC иначе сразу сотрёт диагностику при перезагрузке.
batErrorReturn:
    pop af
    ret
batErrorName:
    ld a,(hl)
    inc hl
    cp ' '
    jp z,batErrorNameNext
    ld c,a
    call j_printChar
batErrorNameNext:
    dec b
    jp nz,batErrorName
    ret
batErrorText:   DB "BAT ",0
batLineText:    DB " LINE ",0
batCodeText:    DB ": ERROR ",0
