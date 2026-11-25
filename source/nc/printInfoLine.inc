;----------------------------------------------------------------------------
; MXOS
;
; Показать имя и размер выбранного файла либо действие для каталога.
;
; вход:
;   Активная панель, её каталог и геометрия NC готовы. Регистровых параметров нет.
;
; выход:
;   Нижняя строка активной панели обновлена. Для EXE/COM отложено чтение заголовка.
;   AF, BC, DE, HL и флаги разрушены.
;----------------------------------------------------------------------------
printInfoLine:
        xor a
        ld (ncStatusPending),a
        ld hl,state.panelA_filesCnt
        call readBytePanel
        or a
        jp z,printInfoEmpty

        call getSelectedFile
        push hl
        ld de,FILE_INFO.attrib
        add hl,de
        ld a,(hl)
        and 10h
        pop hl
        jp z,ncStatusFile
        ld a,(hl)
        cp 05Ch
        ld de,infoRootText
        jp z,ncStatusAction
        ld a,(hl)
        cp '.'
        jp nz,ncStatusDirectory
        inc hl
        ld a,(hl)
        dec hl
        cp '.'
        jp nz,ncStatusDirectory
        push hl
        inc hl
        inc hl
        ld a,(hl)
        pop hl
        cp ' '
        ld de,infoParentText
        jp z,ncStatusAction
ncStatusDirectory:
        ld de,ncTextBuffer
        call compactName
        call ncStatusPad
        ld hl,infoDirectoryText
        ld c,5
        jp ncStatusRight
ncStatusFile:
        call ncStatusExecutable
        call z,ncStatusSchedule
        ld de,ncTextBuffer
        call compactName
        push hl
        call ncStatusPad
        pop hl
        push de
        ld de,FILE_INFO.size
        add hl,de
        ld e,(hl)
        inc hl
        ld d,(hl)
        call ncFormatDecimal
        ld a,(ncDecimalCount)
        ld c,a
        pop de
ncStatusRight:
        ; DE=end of the padded line, C=length of the right-hand field.
        ld a,e
        sub c
        ld e,a
        jp nc,ncStatusRightReady
        dec d
ncStatusRightReady:
        ld b,c
        call ncStatusCopy
        jp ncStatusDraw

ncStatusAction:
        ex de,hl
        ld de,ncTextBuffer
        ld b,16 ; Both messages have 16 characters.
        call ncStatusCopy
        jp ncStatusFinish
printInfoEmpty:
        ld de,ncTextBuffer
ncStatusFinish:
        call ncStatusPad
        jp ncStatusDraw
ncStatusCopy:
        ld a,(hl)
        ld (de),a
        inc hl
        inc de
        dec b
        jp nz,ncStatusCopy
        ret
ncStatusPad:
        ld a,(ncStatusChars)
        ld l,a
        ld h,0
        ld bc,ncTextBuffer
        add hl,bc
        ld a,l
        sub e
        ld b,a
        ret z
        ld a,' '
ncStatusPadByte:
        ld (de),a
        inc de
        dec b
        jp nz,ncStatusPadByte
        ret
ncStatusDraw:
        call ncSetPanelBorder
        ld hl,(P_FILE_NAME_X << 8)+P_FILE_NAME_Y
        call setCursorPosPanel
        ld a,(ncStatusChars)
        ld c,a
        ld b,0
        ld hl,ncTextBuffer
        jp bios_printStringRawLim

infoDirectoryText:  DB '<DIR>'
infoRootText:       DB 'Go to drive root'
infoParentText:     DB 'Go to parent dir'

;----------------------------------------------------------------------------
; MXOS
;
; Определить, имеет ли запись расширение COM или EXE, без обращения к диску.
;
; вход:
;   HL = указатель на FILE_INFO активной панели в текущей странице.
;
; выход:
;   Z = 1 для COM или EXE. HL сохранён, AF, BC, DE и прочие флаги разрушены.
;----------------------------------------------------------------------------
ncStatusExecutable:
        push hl
        ld de,FILE_INFO.ext
        add hl,de
        ld a,(hl)
        and 0DFh
        ld bc,'M'*256+'O'
        cp 'C'
        jp z,ncStatusCheckExtension
        ld bc,'E'*256+'X'
        cp 'E'
        jp nz,ncStatusExtensionResult
ncStatusCheckExtension:
        inc hl
        ld a,(hl)
        and 0DFh
        cp c
        jp nz,ncStatusExtensionResult
        inc hl
        ld a,(hl)
        and 0DFh
        cp b
ncStatusExtensionResult:
        pop hl
        ret

;----------------------------------------------------------------------------
; MXOS
;
; Прочитать заголовок исполняемого файла и определить его платформу.
;
; вход:
;   HL = FILE_INFO файла COM или EXE. Запись принадлежит активной панели.
;
; выход:
;   A = IMAGE_MODE_* либо AUTO при ошибке или неизвестной платформе. BC, DE, HL
;   и флаги разрушены. Диск и сохранённый каталог DOS восстанавливаются.
;----------------------------------------------------------------------------
ncStatusReadPlatform:
        push hl
        call flClear
        pop hl
        ld de,flHandle
        ld b,FILE_NAME_LENGTH+3
        call ncStatusCopy

        ; OPEN_AT принимает диск и кластер каталога активной панели.
        ; Возврат к прежнему диску восстанавливает его сохранённый каталог.
        ld de,0
        MXOS_CALL_REGS MXOS_DOS_FILE_GET_SET_DRIVE
        push af
        ld hl,state.panelA_drive
        call readBytePanel
        ld (flHandle+FH_DRIVE),a
        ld hl,state.panelA_dirLow
        call readBytePanel
        ld (flHandle+FH_SEEK),a
        ld hl,state.panelA_dirHigh
        call readBytePanel
        ld (flHandle+FH_SEEK+1),a
        ld a,FILE_MODE_READ
        ld (flHandle+FH_MODE),a
        ld a,FILE_OP_OPEN_AT
        call flCall
        jp c,ncStatusRestoreDrive
        ld hl,ncStatusHeader
        ld (flHandle+FH_BUFFER),hl
        ld hl,IMAGE_HEADER_SIZE
        ld (flHandle+FH_COUNT),hl
        ld a,FILE_OP_READ
        call flCall
        call flClose
ncStatusRestoreDrive:
        pop bc
        push af
        ld a,b
        MXOS_CALL_REGS MXOS_DOS_FILE_SELECT_DRIVE
        pop bc
        jp c,ncStatusNoPlatform
        ld a,c
        rrca
        jp c,ncStatusNoPlatform
        ld hl,(flHandle+FH_COUNT)
        ld de,IMAGE_HEADER_SIZE
        call bios_cmp_hl_de
        jp nz,ncStatusNoPlatform

        ld hl,ncStatusHeader
        ld de,ncStatusImageMagic
        ld b,5
ncStatusHeaderMagic:
        ld a,(de)
        cp (hl)
        jp nz,ncStatusNoPlatform
        inc hl
        inc de
        dec b
        jp nz,ncStatusHeaderMagic
        ld a,(ncStatusHeader+IMAGE_HEADER_BYTES)
        cp IMAGE_HEADER_SIZE
        jp nz,ncStatusNoPlatform
        ld hl,(flHandle+FILE_DESCRIPTOR.size+2)
        ld a,h
        or l
        jp nz,ncStatusNoPlatform
        ld hl,(ncStatusHeader+10)
        ld a,h
        or l
        jp z,ncStatusNoPlatform
        ld de,IMAGE_HEADER_SIZE
        add hl,de
        jp c,ncStatusNoPlatform
        ex de,hl
        ld hl,(flHandle+FILE_DESCRIPTOR.size)
        call bios_cmp_hl_de
        jp nz,ncStatusNoPlatform
        ld hl,ncStatusHeader
        ld de,ncStatusHeader+IMAGE_HEADER_CHECKSUM-1
    IF NC_BANKED
        call MEM_IO_CHECKSUM
    ELSE
        call bios_calcCS
    ENDIF
        ld hl,(ncStatusHeader+IMAGE_HEADER_CHECKSUM)
        ld d,b
        ld e,c
        call bios_cmp_hl_de
        jp nz,ncStatusNoPlatform
        ld a,(ncStatusHeader+IMAGE_MODE)
        cp IMAGE_MODE_RF+1
        jp nc,ncStatusNoPlatform
        ld b,a
        ld a,(ncStatusHeader+5)
        or a
        jp z,ncStatusCompatibility
        cp IMAGE_FLAG_MAIN
        jp z,ncStatusNative
        cp IMAGE_FLAG_BANK+1
        jp nc,ncStatusNoPlatform
ncStatusNative:
        ld a,IMAGE_MODE_MX
        ret
ncStatusCompatibility:
        ld a,b
        ret
ncStatusNoPlatform:
        xor a
        ret

;----------------------------------------------------------------------------
; MXOS
;
; Нарисовать только метку платформы в её колонке.
;
; вход:
;   A = IMAGE_MODE_MX..IMAGE_MODE_RF. Статус активной панели уже выведен,
;   цвет рамки установлен, инверсия выключена.
;
; выход:
;   Колонка платформы обновлена, курсор печати за меткой. AF, BC, DE, HL
;   и флаги разрушены.
;----------------------------------------------------------------------------
ncStatusDrawPlatform:
        dec a
        add a,a
        add a,a
        add a,a
        ld c,a
        ld b,0
        ld hl,ncStatusPlatforms
        add hl,bc
        ld a,(ncStatusChars)
        cp 21
        ld a,FILE_NAME_LENGTH+1+3+2
        ld bc,6
        jp z,ncStatusShortPlatform
        jp ncStatusPlacePlatform
ncStatusShortPlatform:
        add hl,bc
        dec a
        ld c,2
ncStatusPlacePlatform:
        push hl
        call ncTextAdvance
        add a,P_FILE_NAME_X
        ld h,a
        ld l,P_FILE_NAME_Y
        call setCursorPosPanel
        pop hl
        jp bios_printStringRawLim
ncStatusPlatforms:
        DB 'MXOS  ','MX'
        DB 'STD   ','ST'
        DB 'MON2  ','M2'
        DB 'RAMFOS','RF'
ncStatusImageMagic:
        MXOS_IMAGE_SIGNATURE
ncStatusHeader: DS IMAGE_HEADER_SIZE,0

    IF NC_BANKED
NC_STATUS_QUIET_POLLS = 512
    ELSE
NC_STATUS_QUIET_POLLS = 800
    ENDIF
ncStatusPending: DB 0
ncStatusInput:   DB 0
ncStatusQuiet:   DW 0

;----------------------------------------------------------------------------
; MXOS
;
; Поставить чтение заголовка в ожидание или начать отсчёт тишины заново.
;
; вход:
;   Активная панель содержит выбранный EXE/COM. Регистровых параметров нет.
;
; выход:
;   ncStatusPending = 1, ncStatusQuiet получает NC_STATUS_QUIET_POLLS.
;   BC, DE, HL и флаги сохранены, A разрушен.
;----------------------------------------------------------------------------
ncStatusSchedule:
        ld a,1
        ld (ncStatusPending),a
        push hl
        ld hl,NC_STATUS_QUIET_POLLS
        ld (ncStatusQuiet),hl
        pop hl
        ret

;----------------------------------------------------------------------------
; MXOS
;
; Обслужить отложенное чтение в основном цикле ожидания клавиши.
;
; вход:
;   Регистровых параметров нет. Матрица настроена на опрос всех клавиш.
;
; выход:
;   После непрерывной паузы выполнено одно чтение для текущего выбора.
;   При известной платформе дорисована только её метка. BC, DE, HL и состояние
;   текстового ввода сохранены, AF разрушен.
;----------------------------------------------------------------------------
ncStatusIdle:
        ld a,(ncStatusPending)
        or a
        ret z
        ld a,(ncStatusInput)
        or a
        ret z
        ld a,(IO_KEYB_B)
        or 3
        inc a
        jp nz,ncStatusSchedule
        push hl
        ld hl,(ncStatusQuiet)
        dec hl
        ld (ncStatusQuiet),hl
        ld a,h
        or l
        pop hl
        ret nz
        ld (ncStatusPending),a
        push bc
        push de
        push hl
        call getSelectedFile
        call ncStatusReadPlatform
        or a
        jp z,ncStatusIdleDone
        ld c,a
        ld hl,(bios_vars.cursorY)
        push hl
        ld hl,(bios_vars.inverse)
        push hl
        ld hl,0
        ld (bios_vars.inverse),hl
        ld a,(ncCurrentColor)
        push af
        call ncSetPanelBorder
        ld a,c
        call ncStatusDrawPlatform
        pop af
    IF ENABLE_COLOR
        call ncSetColor
    ENDIF
        pop hl
        ld (bios_vars.inverse),hl
        pop hl
        ld (bios_vars.cursorY),hl
    IF NC_BANKED
        call ncBankFlush
    ENDIF
ncStatusIdleDone:
        pop hl
        pop de
        pop bc
        ret
