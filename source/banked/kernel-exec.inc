; Загрузчик исполняет программу в main, сохраняя BAT и DOS в своём банке.
; Внешний FILEEXEC получает строку клиента. Вызов приложения допускает
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
    ld hl,KD_PATH
    call fileExec
    ; Вложенные запросы уже завершились; результат FILEEXEC — только CF/A.
    call kdCapture
    jp kdFinish

keExternal:
    ld a,(diActive)
    or a
    jp nz,blockArgument
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
    and IMAGE_FLAG_DRIVER
    jp nz,keDriver
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
    jp c,keLoadError
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
    ld hl,bankCallback
    ld de,keRegs
    call bankCallPacket
    xor a
    ret
keDriver:
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
keRangeError:
    pop de
    pop hl
    call blockArgument
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
keBat:
    ; execBat ожидает аргументы на вершине стека и возвращается сюда.
    ld de,keBatDone
    push de
    push hl
    jp execBat
keBatDone:
    jp keDone
keRegs: DS MX_REGS,0
