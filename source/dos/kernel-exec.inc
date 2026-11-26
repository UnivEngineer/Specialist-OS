; Загрузчик исполняет программу в main либо отдельном EXEC-банке.
; Внешний FILE_EXEC получает строку клиента. Вызов приложения допускает
; новые запросы DOS; состояние внешнего диспетчера лежит на стеке DOS.
keRequest:
    ld hl,(kdRegs)
    call kdFetchLine
    jp c,kdBad
    ld a,(kdFunction)
    cp MXOS_DOS_FILE_EXEC_NATIVE & 0FFh
    ld hl,KD_PATH
    jp z,keNativeRequest
    call fileExec
    jp keRequestDone
keNativeRequest:
    call keExternal
keRequestDone:
    ; Вложенные запросы уже завершились; результат FILE_EXEC — только CF/A.
    call kdCapture
    jp kdFinish

fileExecNative:
keExternal:
    ld a,(v_drive)
    push af
    ld de,CMD_LINE
    ld b,MAX_PATH
keCopyCommand:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    cp 020h
    jp c,keCommandCopied
    dec b
    jp nz,keCopyCommand
    scf
    jp keDone
keCommandCopied:
    xor a
    ld (de),a
    ld a,MAX_PATH+2
    sub b
    ld (keCommandSize),a
    ld hl,CMD_LINE
    ld de,v_curFileDescr
    call fileNamePrepare
    jp c,keDone
    ld de,aBat
    push hl
    call cmpFileExt
    pop hl
    jp z,keBat
    ; Явное расширение тоже проверяется: DLL и прочие файлы не приложения.
    push hl
    call cmpFileExt
    pop hl
    jp z,keNativeExtension
    push hl
    call cmpFileExt
    pop hl
    jp nz,keExtensionDenied
keNativeExtension:
    ; Аргументы переводятся в системную строку main до загрузки тела.
    ld de,CMD_LINE
    call sub_hl_de
    ld de,MAIN_COMMAND_ADDR
    add hl,de
    push hl
    ld hl,CMD_LINE
    ld de,MAIN_COMMAND_ADDR
    call keCommandCount
    xor a
    ld (kdPage),a
    call kdStore
    jp c,keLoadError
    ld hl,v_curFileDescr
    call imageOpenModule
    jp c,keLoadError
    ld a,(v_imageHeader+4)
    cp IMAGE_RELOC_VERSION
    ld a,(v_tmpFileDescr+5)
    jp nz,keFixedImage
    cp IMAGE_FLAG_DRIVER
    jp z,keDriver
    call imageError
    jp keLoadError
keFixedImage:
    cp IMAGE_FLAG_BANK
    jp z,keBank
    cp IMAGE_FLAG_DRIVER
    jp z,keDriver
    push hl
    push de
    ld hl,(v_imageHeader+IMAGE_STACK)
    ld de,BIOS_BANK_STACK_TOP-BIOS_BANK_STACK_BOTTOM-64
    call cmp_hl_de
    pop de
    pop hl
    jp c,keMainStackReady
    jp nz,keStackBad
keMainStackReady:
    ; Основная память до переходника RUN; верхние драйверы — явные области.
    push hl
    push de
    ld hl,(v_imageHeader+IMAGE_MEMORY_END)
    push de
    push hl
    ld hl,(v_imageHeader+IMAGE_MIN_HEAP)
    ex de,hl
    pop hl
    add hl,de
    pop de
    jp c,keRangeError
    ld a,d
    cp 08Ch
    push hl
    ld hl,(memorySystemEnd)
    push af
    ld a,(v_imageHeader+IMAGE_POLICY)
    or a
    jp nz,kePreserveFont
    ld hl,APP_MEMORY_END
kePreserveFont:
    pop af
    ex de,hl
    pop hl
    jp c,keCheckEnd
    cp SYSTEM_PROGRAM_ADDR/256
    jp c,keRangeError
    ld de,SYSTEM_PROGRAM_END
keCheckEnd:
    call cmp_hl_de
    jp c,keRangeOk
    jp z,keRangeOk
    ld a,d
    cp SYSTEM_PROGRAM_END/256
    jp z,keRangeError
    pop de                  ; imageOpen entry and load address saved above
    pop hl
    ld hl,keMainShortText
    MXOS_CALL_REGS MXOS_BIOS_PRINT_STRING_BANK
    ld a,MEM_ERROR_NOMEM
    call dosSetError
    jp keLoadError
keMainShortText: DB 'Not enough RAM',10,0
keRangeOk:
    pop hl
    push bc
    push hl
    call moduleMainClaim
    jp c,keMainClaimFailed
    call keCacheClaim
    pop hl
    pop bc
    jp nc,keCacheClaimed
    pop bc
    jp keLoadError
keCacheClaimed:
    push hl
    push bc
    call keReserveImage
    pop bc
    pop hl
    jp c,keReservationFailed
    ld (kdFarCurrent),hl
    ld hl,0
    ld (kiSum),hl
    ld a,1
    ld (kiSumActive),a
    ld (kdFarActive),a
    ld de,KD_DATA
    call imageLoad
    push af
    xor a
    ld (kdFarActive),a
    ld (kiSumActive),a
    pop af
    pop bc
    jp nc,keImageReady
    jp keMainLoadFailed
keReservationFailed:
    pop bc
keMainLoadFailed:
    push af
    call keDropImage
    ld hl,(keCacheToken)
    call keCacheRelease
    pop af
    jp keLoadError
keMainClaimFailed:
    pop hl
    pop bc
    pop bc
    jp keLoadError
keImageReady:
    ; BC=вход в main, DE=аргументы; callback восстанавливает этот пакет.
    pop hl
    ld (keRegs+2),hl
    push bc
    ld hl,keRegs
    ld de,keRegs+MX_REGS
    ; Текущий диск загрузки не должен подменять диск аргументов.
    pop bc
    pop af
    push bc
    call fileSelectDrive
    pop bc
    ; Приложение может запускать DOS и другую программу, не затирая ядро.
    xor a
    ld (kdBusy),a
    ld hl,(keCacheToken)
    push hl
    call appEnter
    jp c,keAppRejected
    call keBindImage
    ld hl,bankCallback
    ld de,keRegs
    call bankCallPacket
    call appLeave
    ; Внутренний вызов FORMAT должен подтвердить запись всех структур.
    push af
    ld a,(pressureBusy)
    or a
    jp nz,keFormatStatus
    pop af
    xor a
    jp keAppRejected
keFormatStatus:
    pop af
keAppRejected:
    push af
    call keDropImage
    pop bc
    pop hl
    push bc
    call keCacheRelease
    call pressureRestore
    pop af
    ret
; Банковое приложение использует тот же жизненный цикл, что DLL.
; Стек: указатель аргументов main, прежний диск. Вызовы ниже локальны DOS.
keBank:
    ld hl,(v_imageHeader+IMAGE_STACK)
    ld de,DOS_BANK_STACK_TOP-DOS_BANK_STACK_BOTTOM-192
    call cmp_hl_de
    jp c,keBankStackReady
    jp nz,keStackBad
keBankStackReady:
    pop hl
    ld de,MAIN_COMMAND_ADDR
    call sub_hl_de
    ld de,DOS_BANK_STACK_BOTTOM
    add hl,de
    push hl
    xor a
    ld (kdBusy),a
    ld hl,CMD_LINE
    call keCommandCount
    ld a,MODULE_TARGET_BANK
    push bc                 ; Длина хранится отдельно на время вложенного запроса.
    MXOS_CALL_REGS MXOS_DOS_MODULE_LOAD
    pop bc
    jp c,keLoadError
    push hl                 ; handle переживает любые вложенные вызовы.
    ld hl,(mdRecord)
    ld de,3
    add hl,de
    ld a,(hl)
    ld (mdBankId),a
    ld hl,CMD_LINE
    ld de,DOS_BANK_STACK_BOTTOM
    call mdInstallPart
    jp c,keBankPrepareFailed
    ; Строка лежит в нижней части стека, недоступной стековым кадрам.
    ld hl,keBankFloor
    ld de,bankStackFloor
    ld bc,2
    call mdInstallPart
    jp c,keBankPrepareFailed
    ld hl,5
    add hl,sp
    ld a,(hl)               ; Восстанавливаем диск до входа приложения.
    call fileSelectDrive
    ld hl,0
    push hl                 ; AF
    push hl                 ; BC
    ld hl,6
    add hl,sp
    ld e,(hl)
    inc hl
    ld d,(hl)
    push de                 ; DE — локальная строка аргументов в EXEC-банке.
    ld hl,0
    push hl                 ; HL
    add hl,sp
    ex de,hl                ; DE — локальный MX_REGS в DOS.
    ld hl,8
    add hl,sp
    ld c,(hl)
    inc hl
    ld b,(hl)
    ld h,b
    ld l,c
    MXOS_CALL_REGS MXOS_DOS_MODULE_CALL
    push af
    ld hl,10
    add hl,sp
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld a,1
    MXOS_CALL_REGS MXOS_DOS_MODULE_UNLOAD
    pop af
    push af
    pop bc
    ld hl,10                ; Пакет и handle больше не нужны.
    add hl,sp
    ld sp,hl
    push bc
    pop af
keBankDone:
    jp keLoadError          ; Снять аргументы, вернуть исходный диск и статус.
keBankPrepareFailed:
    push af
    pop bc
    pop hl
    push bc
    MXOS_CALL_REGS MXOS_DOS_MODULE_UNLOAD
    pop af
    jp keLoadError
keBankFloor: DW DOS_BANK_STACK_BOTTOM+192

;----------------------------------------------------------------------------
; MXOS
; Прочитать размер локальной команды FILE_EXEC с двумя терминаторами.
; вход: keCommandSize — длина, сохранённая keCopyCommand.
; выход: BC = длина, A изменён, DE/HL и флаги сохранены.
;----------------------------------------------------------------------------
keCommandCount:
    ld a,(keCommandSize)
    ld c,a
    ld b,0
    ret
keCommandSize: DB 0

keDriver:
    ld hl,(v_imageHeader+IMAGE_BSS)
    inc hl
    ld a,h
    or l
    jp z,keDriverBad
    ld hl,(v_imageHeader+IMAGE_STACK)
    ld de,DOS_BANK_STACK_TOP-DOS_BANK_STACK_BOTTOM-64
    call cmp_hl_de
    jp c,keDriverStackReady
    jp nz,keStackBad
keDriverStackReady:
    ld a,(diActive)
    or a
    jp nz,keDriverBusy
    call imagePlaceDos
    jp c,keLoadError
    ex de,hl
    ld (diBodyStart),hl
    add hl,bc
    ld (diBodyEnd),hl
    ld hl,(v_imageHeader+IMAGE_MEMORY_BEGIN)
    ld (diStart),hl
    ld de,DOS_ARENA_START
    call cmp_hl_de
    jp c,keDriverBad
    ld hl,(v_imageHeader+IMAGE_MEMORY_END)
    ld (diEnd),hl
    ld de,DOS_BANK_DRIVER_END
    call cmp_hl_de
    jp c,keDriverRangeOk
    jp nz,keDriverBad
keDriverRangeOk:
    ex de,hl
    ld hl,(diStart)
    call imageClaimDos
    jp c,keDriverBusy
    ld hl,(v_tmpFileDescr+6)
    ex de,hl
    call imageLoad
    jp c,keLoadError
    push hl
    call registryFlushH
    pop hl
    jp c,keLoadError
    pop de
    push hl
    ; Аргументы сохраняют тот же NEAR-контракт, но теперь в странице DOS.
    ld hl,CMD_LINE-MAIN_COMMAND_ADDR
    add hl,de
    ex de,hl
    xor a
    ld (kdBusy),a
    inc a
    ld (diActive),a
    ld hl,keDriverReturn
    ex (sp),hl
    jp (hl)
keDriverReturn:
    push af
    xor a
    ld (diActive),a
    pop af
    jp keDone
keStackBad:
    ld a,MEM_ERROR_STACK
    call dosSetError
    jp keLoadError
keDriverBad:
    call blockArgument
    jp keLoadError
keDriverBusy:
    ld a,MEM_ERROR_ACTIVE
    call dosSetError
    jp keLoadError
keRangeError:
    pop de
    pop hl
    call blockArgument
    jp keLoadError
keLoadError:
    pop hl
keDone:
    push af
    pop bc
    pop af
    push bc
    call fileSelectDrive
    call pressureRestore
    pop af
    ret
keExtensionDenied:
    ld a,DOS_ERR_UNSUPPORTED
    call dosSetError
    scf
    jp keDone
keBat:
    ; execBat ожидает аргументы на вершине стека и возвращается сюда.
    ld de,keBatDone
    push de
    push hl
    jp execBat
keBatDone:
    jp keDone

; Верхняя утилита занимает часть арены: токен прежнего конца остаётся
; в локальном стеке DOS на время её исполнения, включая вложенные запуски.
keCacheClaim:
    push hl
    ld hl,0
    ld (keCacheToken),hl
    pop hl
    ld a,h
    cp SYSTEM_PROGRAM_ADDR/256
    jp c,keCacheNoClaim
    ld a,1
    MXOS_CALL_REGS MXOS_BIOS_GLYPH_CACHE
    ret c
    ex de,hl
    ld (keCacheToken),hl
    ex de,hl
keCacheNoClaim:
    xor a
    ret
keCacheRelease:
    ld a,h
    or l
    ret z
    ld a,1
    MXOS_CALL_REGS MXOS_BIOS_GLYPH_CACHE
    ret

; Static MAIN interval is tracked separately from the application's heap.
keReserveImage:
    ld hl,0
    ld (keImageToken),hl
    ld hl,(v_tmpFileDescr+6)
    ld de,APP_MEMORY_END
    call cmp_hl_de
    ret nc                   ; explicit upper utility, cache token handles it
    call keRetireMain
    ld a,(v_imageHeader+IMAGE_POLICY)
    cp IMAGE_POLICY_TRANSIENT
    jp nz,keReserveTracked
    xor a
    ret
keReserveTracked:
    ld hl,(v_imageHeader+IMAGE_BSS)
    inc hl
    ld a,h
    or l
    jp nz,keReserveKnown
    ld hl,0
    ld de,APP_MEMORY_END
    jp keReserveRange
keReserveKnown:
    ld hl,(v_imageHeader+IMAGE_MEMORY_BEGIN)
    push hl
    ld hl,(v_imageHeader+IMAGE_MEMORY_END)
    ex de,hl
    pop hl
keReserveRange:
    call memoryReserveMain
    ret c
    ld hl,(memorySlotPtr)
    ld (keImageToken),hl
    xor a
    ret
keDropImage:
    push af
    ld hl,(keImageToken)
    ld a,h
    or l
    jp z,keDropDone
    ld (hl),0FFh
    ld hl,0
    ld (keImageToken),hl
keDropDone:
    pop af
    ret
keBindImage:
    push bc
    push hl
    ld hl,(keImageToken)
    ld (appImage),hl
    ld a,h
    or l
    jp z,keBindDone
    ld de,5
    add hl,de
    ld a,(appOwner)
    ld (hl),a
    ld (memoryMainOwner),a
    ld hl,(v_imageHeader+IMAGE_MEMORY_END)
    ld (memoryMainStart),hl
    push hl
    ld a,(appDepth)
    dec a
    call appHeapAddress
    ex de,hl
    pop hl
    push hl
    push hl
    ld hl,(v_imageHeader+IMAGE_MIN_HEAP)
    ld b,h
    ld c,l
    pop hl
    add hl,bc
    call appSaveWord
    pop hl
    call appSaveWord
    ld hl,(memorySystemEnd)
    ld (memoryMainEnd),hl
    ld hl,0
    ld (keImageToken),hl
    call memoryPublishTop
keBindDone:
    pop hl
    pop bc
    ret

; Nonreturning MAIN clients can also hand off to a DOS launcher whose child
; needs their MAIN intervals. The command and return stack remain accessible.
memoryRetireMain:
    ld a,(moduleActor)
    or a
    jp nz,memoryDenied
    ld hl,(appImage)
    ld a,h
    or l
    ret z
    call keRetireAll
    jp memoryPublishTop

; FILE_EXEC is a destructive handoff when the caller's MAIN image is replaced.
; Its old static/heap blocks must not constrain the new application's memTop.
; MAIN modules use the strict claim instead and cannot overwrite a live parent.
keRetireMain:
    ld a,(moduleActor)
    or a
    ret nz
    ld hl,(appImage)
    ld a,h
    or l
    ret z
    ld hl,MEM_ALLOC_TABLE
    ld b,MEM_ALLOC_RECORDS
keRetireFind:
    ld a,(hl)
    or a
    jp nz,keRetireNext
    push hl
    push bc
    call mmcReadBlock
    ld a,(appOwner)
    cp (hl)
    jp nz,keRetireNotOwned
    call mmcOverlapSize
    pop bc
    pop hl
    jp c,keRetireAll
    jp keRetireNext
keRetireNotOwned:
    pop bc
    pop hl
keRetireNext:
    ld de,6
    add hl,de
    dec b
    jp nz,keRetireFind
    ret
keRetireAll:
    ld a,(appDepth)
    dec a
    call appHeapAddress
    xor a
    ld (hl),a
    inc hl
    ld (hl),a
    ld hl,MEM_ALLOC_TABLE
    ld b,MEM_ALLOC_RECORDS
    ld a,(appOwner)
    ld c,a
    call appReleaseBlock
    ld hl,0
    ld (appImage),hl
    ret
