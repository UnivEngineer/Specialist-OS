; Загрузчик исполняет программу в main либо отдельном EXEC-банке.
; Внешний FILE_EXEC получает строку клиента. Вызов приложения допускает
; новые запросы DOS; состояние внешнего диспетчера лежит на стеке DOS.
keRequest:
    ld hl,(kdRegs)
    ld de,KD_PATH
    ld bc,MAX_PATH
    call kdFetch
    jp c,kdBad
    ld hl,KD_PATH
    ld b,MAX_PATH
keCheckCommand:
    ld a,(hl)
    cp 020h
    jp c,keCommandReady
    inc hl
    dec b
    jp nz,keCheckCommand
    jp kdBad
keCommandReady:
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
    ld bc,MAX_PATH+2
    xor a
    ld (kdPage),a
    call kdStore
    jp c,keLoadError
    ld hl,v_curFileDescr
    call imageOpen
    jp c,keLoadError
    ld a,(v_tmpFileDescr+5)
    cp IMAGE_FLAG_BANK
    jp z,keBank
    cp IMAGE_FLAG_DRIVER
    jp z,keDriver
    ; Основная память до переходника RUN; верхние драйверы — явные области.
    push hl
    push de
    ld h,b
    ld l,c
    add hl,de
    jp c,keRangeError
    ld a,d
    cp 08Ch
    ld de,08C00h
    jp c,keCheckEnd
    cp SYSTEM_PROGRAM_ADDR/256
    jp c,keRangeError
    ld de,SYSTEM_PROGRAM_END
keCheckEnd:
    call cmp_hl_de
    jp c,keRangeOk
    jp nz,keRangeError
keRangeOk:
    pop hl
    push bc
    push hl
    call keCacheClaim
    pop hl
    pop bc
    jp nc,keCacheClaimed
    pop bc
    jp keLoadError
keCacheClaimed:
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
    push af
    ld hl,(keCacheToken)
    call keCacheRelease
    pop af
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
    ld hl,bankCallback
    ld de,keRegs
    call bankCallPacket
    call appLeave
    xor a
keAppRejected:
    push af
    pop bc
    pop hl
    push bc
    call keCacheRelease
    pop af
    ret
; Банковое приложение использует тот же жизненный цикл, что DLL.
; Стек: указатель аргументов main, прежний диск. Вызовы ниже локальны DOS.
keBank:
    pop hl
    ld de,MAIN_COMMAND_ADDR
    call sub_hl_de
    ld de,DOS_BANK_STACK_BOTTOM
    add hl,de
    push hl
    xor a
    ld (kdBusy),a
    ld hl,CMD_LINE
    ld a,MODULE_TARGET_BANK
    MXOS_CALL MXOS_DOS_MODULE_LOAD
    jp c,keLoadError
    push hl                 ; handle переживает любые вложенные вызовы.
    ld hl,(mdRecord)
    ld de,3
    add hl,de
    ld a,(hl)
    ld (mdBankId),a
    ld hl,CMD_LINE
    ld de,DOS_BANK_STACK_BOTTOM
    ld bc,MAX_PATH+2
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
    MXOS_CALL MXOS_DOS_MODULE_CALL
    push af
    ld hl,10
    add hl,sp
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld a,1
    MXOS_CALL MXOS_DOS_MODULE_UNLOAD
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
    MXOS_CALL MXOS_DOS_MODULE_UNLOAD
    pop af
    jp keLoadError
keBankFloor: DW DOS_BANK_STACK_BOTTOM+192

keDriver:
    ld a,(moduleDosOwner)
    or a
    jp nz,keDriverBusy
    ld a,(diActive)
    or a
    jp nz,keDriverBad
    ; Стек: адрес аргументов main, прежний диск. Код не затрагивает main.
    ld a,d
    cp DOS_BANK_DRIVER_ADDR/256
    jp nz,keDriverBad
    ld a,e
    or a
    jp nz,keDriverBad
    ld h,b
    ld l,c
    add hl,de
    jp c,keDriverBad
    ld (diEnd),hl
    ld de,DOS_BANK_DRIVER_END
    call cmp_hl_de
    jp c,keDriverRangeOk
    jp nz,keDriverBad
keDriverRangeOk:
    ld de,DOS_BANK_DRIVER_ADDR
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
keRegs: DS MX_REGS,0

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
    MXOS_CALL MXOS_BIOS_GLYPH_CACHE
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
    MXOS_CALL MXOS_BIOS_GLYPH_CACHE
    ret
keCacheToken: DW 0
