; RUN.COM v2: общий запуск COM/EXE/RKS в MX/STD/MON/RAMFOS.
; DE=аргументы, CF=ошибка до передачи. Регистры/флаги разрушаются.
; Тело читается прямо по месту; подготовка и меню не остаются резидентными.
; Разбор/меню в main; финальный загрузчик 8DB0h, переходник 8C00h или FONT_ADDR.
; Защищённая область RUN в main: 8C00h–8DFFh, доступна и в STD.
; Она переживает загрузку приложения с 0000h и не входит в его memtop.
;   8C00–8CFF: переходник MX/STD (226 байт) либо продолжение Monitor-2.
;   8D00–8D7F: аргументы приложения, 128 байт.
;   8D80–8DAA: запрос загрузки тела, ожидаемая сумма и исходный диск, 43 байта.
;   8DAB–8DAF: резерв параметров, 5 байт.
;   8DB0–8DEB: финальное чтение/проверка тела, 60 байт.
;   8DEC–8DFF: резерв продолжения, 20 байт.
; Подмена MON/RF использует FONT_ADDR; верхняя программа RF — экран AB00h.
; До запуска приложения 8C00h также служит временным ID-кодом FLASHSYS.
    INCLUDE "../include/mxos.inc"
    INCLUDE "../include/ramfos.inc"
IMAGE_FLAGS = IMAGE_FLAG_MAIN
RUN_BODY_REQUEST = RAMFOS_CONTEXT
RUN_LOAD_STAGE = RAMFOS_CONTEXT+48
    MACRO GET_DE address
        push hl
        ld hl,(address)
        ex de,hl
        pop hl
    ENDM
    MACRO PUT_DE address
        ex de,hl
        ld (address),hl
        ex de,hl
    ENDM
    ORG 0000h
start:
    ld hl,0
    ld (cacheToken),hl
    add hl,sp
    ld (callerSP),hl
    ld a,(bios_upper.koi7)
    ld (callerKoi),a
    push de
    ld e,2
    call bios_fileGetSetDrive
    ld (callerDrive),a
    pop de
    xor a
    call bios_setFontEncoding
    ex de,hl
    xor a
    ld (appName),a
    ld de,appArgs
    ld b,127
    ld a,0FFh
    ld (mode),a
; Один проход: ключи RUN удаляются, остальные аргументы сохраняют порядок.
nextArgument:
    push bc
    push de
    call bios_argNext
    jp c,argumentsDone
    push hl                 ; Курсор следующего слова, отдельно от его начала.
    ex de,hl
    ld a,(hl)
    cp '-'
    jp nz,argument
    inc hl
    ld de,keyKOI7
    ld c,4
    call option
    jp nz,runtimeOption
    ld a,0FFh
    ld (launchKoi),a
    inc hl
    inc hl
    inc hl
    inc hl
    jp endOption
runtimeOption:
    ld de,keyMX
    ld c,2
    call option
    ld a,0
    jp z,shortOption
    ld de,keyRF
    call option
    ld a,3
    jp z,shortOption
    ld de,keySTD
    ld c,3
    call option
    ld a,1
    jp z,longOption
    ld de,keyMON
    call option
    jp nz,unknownOption
    ld a,2
    ld (mode),a
    inc hl
    inc hl
    inc hl
    ld a,(hl)
    cp ':'
    jp nz,endOption
    inc hl
    call coreName
    jp c,argumentError
    jp endOption
shortOption:
    ld (mode),a
    inc hl
    inc hl
    jp endOption
longOption:
    ld (mode),a
    inc hl
    inc hl
    inc hl
endOption:
    ld a,(hl)
    cp ' '+1
    jp c,argumentParsed
    cp ' '
    jp nz,argumentError
argumentParsed:
    pop hl
    pop de
    pop bc
    jp nextArgument
unknownOption:
    dec hl
    ld a,(appName)
    or a
    jp z,argumentError
argument:
    ld a,(appName)
    or a
    jp nz,programArgument
filename:
    push hl
    ld de,appName
    ld b,MAX_PATH-1
nameCopy:
    ld a,(hl)
    cp ' '+1
    jp c,nameDone
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,nameCopy
    pop hl
    jp argumentError
nameDone:
    xor a
    ld (de),a
    pop hl
    ld de,descriptor
    call bios_fileNamePrepare
    jp c,argumentError
    jp argumentParsed
programArgument:
    pop af                  ; Курсор восстановит проход копирования слова.
    pop de
    pop bc
    ld a,b
    cp 127
    jp z,argsCopy
    or a
    jp z,usage
    ld a,' '
    ld (de),a
    inc de
    dec b
argsCopy:
    ld a,(hl)
    cp ' '+1
    jp c,nextArgument
    inc b
    dec b
    jp z,usage
    ld (de),a
    inc hl
    inc de
    dec b
    jp argsCopy
argumentError:
    pop hl
    pop de
    pop bc
    jp usage
argumentsDone:
    pop de
    pop bc
    xor a
    ld (de),a
    ld a,(appName)
    or a
    jp z,usage
    ld a,(descriptor+FILE_DESCRIPTOR.ext)
    and 0DFh
    cp 'R'
    jp z,rksOpen
    cp 'C'
    jp z,nativeOpen
    cp 'E'
    jp nz,badImage
nativeOpen:
    ld hl,descriptor
    ld a,16
    call bios_imageOpen
    jp c,badImage
    ld (appEntry),hl
    PUT_DE appLoad
    ld h,b
    ld l,c
    ld (appSize),hl
    ; Заголовок контейнера принадлежит RUN; общая загрузка тела его не разбирает.
    ld hl,(descriptor+FILE_DESCRIPTOR.firstCluster)
    ld (rksCursor),hl
    ld hl,0
    ld (rksCursor+2),hl
    ld hl,rksHeader
    ld bc,16
    call rksRead
    jp c,badImage
    ld a,(rksHeader+5)
    or a
    jp nz,nativeForward
    ld hl,(rksHeader+12)
    ld (expectedChecksum),hl
    ld a,(mode)
    inc a
    jp nz,ready
    jp chooseMode
; Подтверждённый MAIN/DRIVER/BANK исполняет DOS без CLI/PATH/ассоциаций.
; main-приложение может затереть RUN: продолжение заранее переносится в 8C00h.
nativeForward:
    ld a,(launchKoi)
    call bios_setFontEncoding
    ld hl,appName
    ld de,coreInput
    ld b,MAX_PATH-1
    call nativeCopyArgument
    jp c,usage
    ld a,' '
    ld (de),a
    inc de
    dec b
    jp z,usage
    ld hl,appArgs
    call nativeCopyArgument
    jp c,usage
    xor a
    ld (de),a
    ld hl,coreInput
    ld de,RAMFOS_ARGS
    ld bc,MAX_PATH
    call copyBytes
    ld hl,nativeBridge
    ld de,RAMFOS_BRIDGE
    ld bc,nativeBridgeEnd-nativeBridge
    call copyBytes
    jp RAMFOS_BRIDGE
nativeCopyArgument:
    ld a,(hl)
    or a
    ret z
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,nativeCopyArgument
    scf
    ret
nativeBridge:
    ld hl,RAMFOS_ARGS
    MXOS_CALL MXOS_DOS_FILE_EXEC_NATIVE
    jp nc,0C000h
    ld hl,RAMFOS_BRIDGE+nativeError-nativeBridge
    call bios_printString
    call bios_getch
    jp 0C000h
nativeError: DB 'NATIVE LOAD FAILED',13,10,0
nativeBridgeEnd:
    ASSERT nativeBridgeEnd-nativeBridge <= 256

rksOpen:
    ld hl,descriptor
    call bios_fileLoadInfo
    jp c,notFound
    ld hl,(descriptor+FILE_DESCRIPTOR.size+2)
    ld a,h
    or l
    jp nz,badImage
    ld hl,(descriptor+FILE_DESCRIPTOR.size)
    ld (fileSize),hl
    ld de,6
    call bios_cmp_hl_de
    jp c,badImage
    ld hl,(descriptor+FILE_DESCRIPTOR.firstCluster)
    ld (rksCursor),hl
    ld hl,0
    ld (rksCursor+2),hl
    ld hl,rksHeader
    ld bc,4
    call rksRead
    jp c,badImage
    ld hl,(rksHeader)
    ex de,hl
    ld hl,(rksHeader+2)
    PUT_DE appLoad
    PUT_DE appEntry
    call bios_sub_hl_de
    jp c,badImage
    inc hl
    ld (appSize),hl
    ld a,h
    or l
    jp z,badImage
    ld de,6
    add hl,de
    jp c,badImage
    GET_DE fileSize
    call bios_cmp_hl_de
    jp nz,badImage
    ld a,(mode)
    inc a
    jp nz,ready
    ld a,2
setDefault:
    ld (mode),a
    jp ready
chooseMode:
    ld hl,txtTitle
    call bios_printString
    ld hl,appName
    call bios_printString
    ld hl,txtLoad
    call bios_printString
    ld hl,(appLoad)
    call bios_printHexWord
    ld hl,txtEntry
    call bios_printString
    ld hl,(appEntry)
    call bios_printHexWord
    ld hl,txtModes
    call bios_printString
modeKey:
    call bios_getch
    sub '1'
    cp 5
    jp nc,modeKey
    cp 4
    jp z,customMode
    ld hl,modeMap
    ld e,a
    ld d,0
    add hl,de
    ld a,(hl)
    jp setDefault
customMode:
    ld hl,txtCustom
    call bios_printString
    ld hl,coreInput
    ld de,coreInput+MAX_PATH-1
    call bios_input
    ld hl,coreInput
    call coreName
    jp c,usage
    ld a,2
    jp setDefault
    jp ready
ready:
upperSource = ready ; Метка прежнего CPU-стенда; верхнего размещения больше нет.
    ld hl,(appLoad)
    GET_DE appSize
    add hl,de
    jp c,badImage
    ld a,(mode)
    cp 3
    jp z,rfRange
    ld de,LEGACY_APP_MEMORY_END
    ld a,(descriptor+FILE_DESCRIPTOR.ext)
    and 0DFh
    cp 'R'
    jp z,rangeLowReady
    ld a,(mode)
    or a
    jp nz,rangeLowReady
    push hl
    call bios_getMemTop
    ld de,APP_MEMORY_END-1
    call bios_cmp_hl_de
    jp c,rangeMemTopReady
    ex de,hl
rangeMemTopReady:
    inc hl
    ex de,hl
    pop hl
rangeLowReady:
    ld a,(appLoad+1)
    cp 0C0h
    jp c,rangeEnd
    ld a,(mode)
    or a
    jp nz,badImage
    ld a,(appLoad+1)
    cp SYSTEM_PROGRAM_ADDR >> 8
    jp c,badImage
    ld de,SYSTEM_PROGRAM_END
    jp rangeEnd
rfRange:
    ld de,LEGACY_APP_MEMORY_END
    ld a,(appLoad+1)
    cp 0D3h
    jp c,rangeEnd
    ld de,RAMFOS_HIGH_ADDR
rangeEnd:
    call bios_cmp_hl_de
    jp c,rangeOK
    jp nz,badImage
rangeOK:
    ld a,(mode)
    or a
    jp nz,cacheRangeReady
    ld hl,(appLoad)
    ld a,h
    cp SYSTEM_PROGRAM_ADDR/256
    jp c,cacheRangeReady
    ld a,1
    MXOS_CALL MXOS_BIOS_GLYPH_CACHE
    jp c,badImage
    ex de,hl
    ld (cacheToken),hl
cacheRangeReady:
    ; Сохранить описание приложения до поиска сменного ядра.
    ld e,2
    call bios_fileGetSetDrive
    ld (appDrive),a
    ld hl,descriptor
    ld de,bodyRequest
    ld bc,FILE_DESCR_SIZE
    call copyBytes
    ld hl,(appLoad)
    ld (bodyRequest+FILE_BODY_IO.buffer),hl
    ld a,16
    ld (bodyRequest+FILE_BODY_IO.header),a
    xor a
    ld (bodyRequest+FILE_BODY_IO.tail),a
    ld (staged),a
    ; Верхняя программа RF может затереть ещё выполняемый BIOS.
    ; Проверенное тело сначала читается в 0000h, затем переносится без BIOS.
    ld a,(mode)
    cp 3
    jp nz,bodyDestinationReady
    ld a,(appLoad+1)
    cp 0D3h
    jp c,bodyDestinationReady
    ld a,1
    ld (staged),a
    ld hl,0
    ld (bodyRequest+FILE_BODY_IO.buffer),hl
bodyDestinationReady:
    ld a,(descriptor+FILE_DESCRIPTOR.ext)
    and 0DFh
    cp 'R'
    jp nz,bodyPrepared
    ld a,4
    ld (bodyRequest+FILE_BODY_IO.header),a
    ld a,2
    ld (bodyRequest+FILE_BODY_IO.tail),a
    ld hl,RUN_BODY_REQUEST+FILE_BODY_IO
    ld (bodyRequest+FILE_BODY_IO.trailer),hl
bodyPrepared:
    ld a,(mode)
    cp 2
    jp c,install
    ld hl,corePath
    cp 3
    jp nz,openCore
    ld hl,rfPath
openCore:
    ld de,descriptor
    call bios_fileNamePrepare
    ld hl,descriptor
    call bios_fileLoadInfo
    jp c,notFound
    ld hl,(descriptor+FILE_DESCRIPTOR.size+2)
    ld a,h
    or l
    jp nz,badImage
    ld hl,(descriptor+FILE_DESCRIPTOR.size)
    ld de,01000h
    ld a,(mode)
    cp 3
    jp nz,coreSize
    ld de,RAMFOS_IMAGE_SIZE
coreSize:
    call bios_cmp_hl_de
    jp nz,badImage
    xor a
    ld (IO_COLOR),a
    call bios_clearScreenOld
    ld hl,descriptor
    ld de,RAMFOS_TEMP
    call bios_fileLoad2
    jp c,notFound
    ld a,(mode)
    cp 3
    jp z,install
    ; Два известных пролога Monitor-2: оригинальный JMP и старый POP B.
    ld hl,RAMFOS_TEMP+4
    ld a,(RAMFOS_TEMP)
    cp 0C1h
    jp z,monitorOld
    cp 0C3h
    jp nz,badImage
    dec hl
monitorOld:
    ld a,(hl)
    cp 031h
    jp nz,badImage
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    PUT_DE monSP
    inc hl
    inc hl
    inc hl
    inc hl
    inc hl
    ; После LXI SP идут MVI A,03h; STA F7FAh; PUSH B; JMP init.
    inc hl
    inc hl
    ld a,(hl)
    cp 0C3h
    jp nz,badImage
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    PUT_DE monInit
install:
    ; Исходный том восстанавливается до последнего копирования тела/ядра.
    ld a,(appDrive)
    ld e,1
    call bios_fileGetSetDrive
    ld a,(callerDrive)
    ld (bodyCallerDrive),a
    ; Кодировка независима от MX/STD; сменные ядра задают её сами.
    ld a,(launchKoi)
    call bios_setFontEncoding
    ; Переходник остаётся доступным после переключения в STD.
    ld hl,RAMFOS_BRIDGE
    ld de,RAMFOS_ARGS
    ld bc,RAMFOS_BRIDGE_SP
bridgeLowImage:
    ld (bridgeBase),hl
    PUT_DE argAddress
    ld h,b
    ld l,c
    ld (bridgeTemporarySP+1-bridge+bridgeImage),hl
    ld a,(mode)
    cp 2
    jp c,prepareBridge
    ld hl,FONT_ADDR
    ld a,(staged)
    or a
    jp z,coreBridgeReady
    ; Верхнее тело RF может перекрывать и сам шрифт: нужен экранный мост.
    ld hl,RAMFOS_VIDEO_BRIDGE
coreBridgeReady:
    ld (bridgeBase),hl
    ld hl,RAMFOS_VIDEO_SP
    ld (bridgeTemporarySP+1-bridge+bridgeImage),hl
prepareBridge:
    ld hl,0
    ld (bridgeSource+1-bridge+bridgeImage),hl
    ld a,(staged)
    or a
    jp z,bridgeNoBodyCopy
    ld hl,(appSize)
bridgeNoBodyCopy:
    ld (bridgeSize+1-bridge+bridgeImage),hl
    ld hl,(appLoad)
    ld (bridgeLoad+1-bridge+bridgeImage),hl
    ld hl,(appEntry)
    ld (bridgeEntry+1-bridge+bridgeImage),hl
    ld (monAfterInitEntry+1),hl
    ld hl,(callerSP)
    ld (bridgeCallerSP+1-bridge+bridgeImage),hl
    ld hl,(cacheToken)
    ld (bridgeCacheLimit+1-bridge+bridgeImage),hl
    ld hl,(callerSP)
    ld (bridgeGameSP+1-bridge+bridgeImage),hl
    ld hl,(monSP)
    ld (monAfterInitSP+1),hl
    ld hl,(monInit)
    ld (bridgeInit+1-bridge+bridgeImage),hl
    ld a,(mode)
    ld (bridgeMode-bridge+bridgeImage),a
    ld a,(callerKoi)
    ld (bridgeRestoreKoi+1-bridge+bridgeImage),a
    ld hl,(argAddress)
    ld (bridgeArguments+1-bridge+bridgeImage),hl
    ld (monAfterInitArgs+1),hl
    ; Все внутренние адреса переходника корректируются один раз при установке.
    ld hl,(bridgeBase)
    ld de,bridge
    call bios_sub_hl_de
    ld (bridgeDelta),hl
    ld hl,bridgeRelocations
    ld a,bridgeRelocationsEnd-bridgeRelocations
bridgeRelocate:
    ld e,(hl)
    ld d,0
    push hl
    ld hl,bridgeImage
    add hl,de
    push hl
    ld c,(hl)
    inc hl
    ld b,(hl)
    ld hl,(bridgeDelta)
    add hl,bc
    ex de,hl
    pop hl
    ld (hl),e
    inc hl
    ld (hl),d
    pop hl
    inc hl
    dec a
    jp nz,bridgeRelocate
    ; MON не возвращается в экранный код: его инициализация стирает экран.
    ld a,(mode)
    cp 2
    jp c,copyBridge
    xor a
    ld (IO_COLOR),a
    ld hl,coldStub
    ld de,RAMFOS_BRIDGE
    ld bc,monAfterInitEnd-coldStub
    call copyBytes
copyBridge:
    ld hl,appArgs
    GET_DE argAddress
    ld bc,128
    call copyBytes
    ld hl,bridgeImage
    GET_DE bridgeBase
    ld bc,bridgeEnd-bridge
    call copyBytes
    ; Параметры и короткое продолжение переживают загрузку тела с 0000h.
    ld hl,bodyRequest
    ld de,RUN_BODY_REQUEST
    ld bc,bodyRequestEnd-bodyRequest
    call copyBytes
    ld hl,(bridgeBase)
    ld (loadStageBridge+1),hl
    ld hl,loadStage
    ld de,RUN_LOAD_STAGE
    ld bc,loadStageEnd-loadStage
    call copyBytes
runBodyReady:
    jp RUN_LOAD_STAGE

; Формат RKS целиком принадлежит RUN. HL=буфер, BC=точная длина.
; Курсор используется в нижнем RUN только до окончательной загрузки тела.
rksRead:
    ld de,rksCursor
    ld a,2
    MXOS_CALL MXOS_DOS_FILESTREAM
    ret
; BC=вычисленная сумма, HL=два байта суммы из RKS; CF=несовпадение.
rksChecksum:
    ld e,(hl)
    inc hl
    ld d,(hl)
    ld h,b
    ld l,c
    call bios_cmp_hl_de
    ret z
    scf
    ret
coldStub:
    xor a
    ld (IO_PAGE_ROM-0800h),a
    jp 0
coldStubEnd:
; Monitor-2 может очистить и экранный код, и весь экранный стек.
; После его инициализации продолжаем из 8C00h, затем ставим стек игры.
monAfterInitSP:
    ld sp,0
    ld hl,RAMFOS_BRIDGE
    push hl
monAfterInitArgs:
    ld de,0
monAfterInitEntry:
    ld hl,0
    jp (hl)
monAfterInitEnd:
option:
    push hl
    call bios_stricmp
    pop hl
    ret
; HL — имя до пробела/терминатора, сохраняется позиция разделителя.
coreName:
    ld a,(hl)
    cp 021h
    jp c,coreBad
    ld b,MAX_PATH-1
    ld de,corePath
    push hl
    inc hl
    ld a,(hl)
    cp ':'
    pop hl
    jp z,coreCopyStart
    ld a,'A'
    ld (de),a
    inc de
    ld a,':'
    ld (de),a
    inc de
    ld a,05Ch
    ld (de),a
    inc de
    ld b,MAX_PATH-4
coreCopyStart:
    ld c,0
coreCopy:
    ld a,(hl)
    cp ' '+1
    jp c,coreEnd
    cp '.'
    jp nz,coreChar
    inc c
coreChar:
    ld (de),a
    inc de
    inc hl
    dec b
    jp nz,coreCopy
    scf
    ret
coreEnd:
    ld a,c
    or a
    jp nz,coreTerm
    ld a,b
    cp 4
    jp c,coreBad
    push hl
    ld hl,monSuffix
    ld bc,4
    call copyBytes
    pop hl
coreTerm:
    xor a
    ld (de),a
    ret
coreBad:
    scf
    ret
copyBytes:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,copyBytes
    ret
usage:
    ld hl,txtUsage
    jp error
notFound:
    ld hl,txtNotFound
    jp error
badImage:
    ld hl,txtBad
error:
    push hl
    ld hl,(cacheToken)
    ld a,h
    or l
    jp z,errorCacheReady
    ld a,1
    MXOS_CALL MXOS_BIOS_GLYPH_CACHE
errorCacheReady:
    pop hl
    ld a,COLOR_BIOS
    ld (IO_COLOR),a
    call bios_printString
    ld a,(callerKoi)
    call bios_setFontEncoding
    scf
    ret

bridgeImage:
bridge:
bridgeTemporarySP:
    ld sp,RAMFOS_BRIDGE_SP
bridgeSource:
    ld hl,0
bridgeSize:
    ld bc,0
bridgeLoad:
    ld de,0
    ld a,b
    or c
bridgeReloc0:
    call nz,bridgeMove
bridgeReloc1:
    ld a,(bridgeMode)
    cp 2
bridgeReloc2:
    jp c,bridgeNative
    ld hl,RAMFOS_TEMP
    ld de,0C000h
    ld bc,01000h
    cp 3
bridgeReloc3:
    jp nz,bridgeCore
    ld bc,RAMFOS_CORE_SIZE
bridgeCore:
bridgeReloc4:
    call bridgeMove
bridgeReloc5:
    ld a,(bridgeMode)
    cp 3
bridgeReloc6:
    jp nz,bridgeSTD
    ld hl,RAMFOS_TEMP+RAMFOS_CORE_SIZE
    ld de,RAMFOS_HIGH_ADDR
    ld bc,RAMFOS_HIGH_SIZE
bridgeReloc7:
    call bridgeMove
bridgeReloc8:
    jp bridgeEntry
bridgeNative:
    or a
bridgeReloc9:
    jp z,bridgeStart
    ld a,0FFh
    call bios_setFontEncoding
bridgeSTD:
    ld a,0B6h
    ld (IO_TIMER_MODE),a
    ld a,076h
    ld (IO_TIMER_MODE),a
    ld a,036h
    ld (IO_TIMER_MODE),a
    ld a,00Ah
    ld (IO_KEYB_MODE),a
    ld a,3
    ld (IO_PAGE_STD),a
    ld (IO_COLOR-07FEh),a
    ld a,082h
    ld (0F803h),a
    xor a
    ld (0F802h),a
bridgeReloc10:
    ld a,(bridgeMode)
    cp 2
bridgeReloc11:
    jp nz,bridgeStart
bridgeMonSP:
    ld sp,STACK_ADDR
    ld hl,RAMFOS_BRIDGE+coldStubEnd-coldStub
    push hl
bridgeInit:
    jp 0C444h
bridgeStart:
bridgeGameSP:
    ld sp,0
bridgeReloc12:
    ld hl,bridgeReturn
    push hl
bridgeEntry:
    ld hl,0
bridgeArguments:
    ld de,RAMFOS_ARGS
bridgeReloc13:
    ld a,(bridgeMode)
    cp 3
    jp z,RAMFOS_START
    jp (hl)
bridgeReturn:
bridgeReloc14:
    ld a,(bridgeMode)
    or a
bridgeReloc15:
    jp z,bridgeReturnedMX
    ld (IO_PAGE_RAM-0800h),a
bridgeReturnedMX:
bridgeCallerSP:
    ld sp,0
    push af                 ; Режим нужен после восстановления кэша и KOI.
bridgeCacheLimit:
    ld hl,0
    ld a,h
    or l
bridgeReloc19:
    jp z,bridgeCacheDone
    ld a,1
    MXOS_CALL MXOS_BIOS_GLYPH_CACHE
bridgeCacheDone:
bridgeRestoreKoi:
    ld a,0
    call bios_setFontEncoding
    ld a,COLOR_BIOS
    ld (IO_COLOR),a
    pop af
    cp 2
    jp c,0C000h             ; MX/STD: нижний RUN и код оболочки могли быть затёрты.
    or a
    ret
; Неперекрывающиеся диапазоны: тело 0000h -> верхнее ОЗУ,
; ядро 9000h -> C000h/F800h. BC>0; AF/BC/DE/HL разрушаются.
; Свой копировальщик, без вызовов уже заменяемого DOS.
; Полный блок: 135 тактов/4 байта, четыре чтения и четыре записи.
bridgeMove:
    ld a,c
    and 3
bridgeReloc16:
    jp z,bridgeCopyBlocks
bridgeCopyByte:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec bc
    ld a,c
    and 3
bridgeReloc17:
    jp nz,bridgeCopyByte
    ld a,b
    or c
    ret z
bridgeCopyBlocks:
    DUP 4
        ld a,(hl)
        ld (de),a
        inc hl
        inc de
    EDUP
    dec bc
    dec bc
    dec bc
    dec bc
    ld a,b
    or c
bridgeReloc18:
    jp nz,bridgeCopyBlocks
    ret
bridgeMode: DB 0
bridgeEnd:
    ASSERT bridgeEnd-bridge <= 0100h

bridgeRelocations:
    DB bridgeReloc0+1-bridge
    DB bridgeReloc1+1-bridge
    DB bridgeReloc2+1-bridge
    DB bridgeReloc3+1-bridge
    DB bridgeReloc4+1-bridge
    DB bridgeReloc5+1-bridge
    DB bridgeReloc6+1-bridge
    DB bridgeReloc7+1-bridge
    DB bridgeReloc8+1-bridge
    DB bridgeReloc9+1-bridge
    DB bridgeReloc10+1-bridge
    DB bridgeReloc11+1-bridge
    DB bridgeReloc12+1-bridge
    DB bridgeReloc13+1-bridge
    DB bridgeReloc14+1-bridge
    DB bridgeReloc15+1-bridge
    DB bridgeReloc16+1-bridge
    DB bridgeReloc17+1-bridge
    DB bridgeReloc18+1-bridge
    DB bridgeReloc19+1-bridge
bridgeRelocationsEnd:

keyMX: DB "MX"
keyKOI7: DB "KOI7"
keyRF: DB "RF"
keySTD: DB "STD"
keyMON: DB "MON"
modeMap: DB 0,1,3,2
monSuffix: DB ".MON"
rfPath: DB "A:",05Ch,"RAMFOS.MON",0
corePath: DB "A:",05Ch,"MONITOR2.MON",0
    BLOCK MAX_PATH-($-corePath),0
appArgs: BLOCK 128,0
mode: DB 0FFh
callerSP: DW 0
appLoad: DW 0
appEntry: DW 0
appSize: DW 0
rksCursor: FILE_STREAM_CURSOR
rksHeader: BLOCK 16,0
bridgeBase: DW 0
bridgeDelta: DW 0
argAddress: DW 0
staged: DB 0
callerDrive: DB 0
callerKoi: DB 0
launchKoi: DB 0
fileSize: DW 0
appDrive: DB 0
monSP: DW 07FFFh
monInit: DW 0C444h
descriptor: BLOCK FILE_DESCR_SIZE,0
txtUsage: DB 0Ah,"RUN FILE [-MX|-STD|-RF|-MON[:FILE]] [-KOI7]",0
txtNotFound: DB 0Ah,"FILE NOT FOUND",0
txtBad: DB 0Ah,"BAD IMAGE OR ADDRESS RANGE",0
upperEnd: ; Граница прежнего CPU-стенда, все эти байты теперь в нижнем RUN.
bodyRequest: FILE_BODY_IO
expectedChecksum: DW 0
bodyCallerDrive: DB 0
bodyRequestEnd:
    ASSERT bodyRequestEnd-bodyRequest <= RUN_LOAD_STAGE-RUN_BODY_REQUEST

; Последний этап только читает тело, проверяет сумму и входит в переходник.
; Ошибка после затирания RUN требует bootstrap: он восстановит BIOS/шрифт.
; Адреса внутри блока отсутствуют, кроме патча последнего JP.
loadStage:
    ld hl,RUN_BODY_REQUEST
    MXOS_CALL MXOS_BIOS_FILE_BODY
    jp c,RUN_LOAD_STAGE+loadStageError-loadStage
    ld a,(RUN_BODY_REQUEST+FILE_BODY_IO+2)
    ld e,1
    call bios_fileGetSetDrive
    jp c,RUN_LOAD_STAGE+loadStageError-loadStage
    ld hl,(RUN_BODY_REQUEST+FILE_BODY_IO.buffer)
    push hl
    GET_DE RUN_BODY_REQUEST+FILE_BODY_IO.actual
    add hl,de
    dec hl
    ex de,hl
    pop hl
    call bios_calcCS
    ld hl,(RUN_BODY_REQUEST+FILE_BODY_IO)
    ld d,b
    ld e,c
    call bios_cmp_hl_de
    jp nz,RUN_LOAD_STAGE+loadStageError-loadStage
loadStageBridge:
    jp 0
loadStageError:
    xor a
    ld (IO_PAGE_ROM),a
    jp 0
loadStageEnd:
    ASSERT RUN_LOAD_STAGE+loadStageEnd-loadStage <= RAMFOS_CONTEXT+128
    ASSERT $ < RAMFOS_BRIDGE

txtTitle: DB 0Ah,"MXOS2 EXECUTABLE LAUNCHER V2.0",0Ah,0
txtLoad: DB ": LOAD ADDRESS ",0
txtEntry: DB ", RUN ADDRESS ",0
txtModes: DB 0Ah,"CHOOSE THE RUNTIME MODE:",0Ah
    DB "1 - MX:  MX  HARDWARE, MXOS2 CORE",0Ah
    DB "2 - STD: STD HARDWARE, MXOS2 CORE",0Ah
    DB "3 - RF:  MX  HARDWARE, RAMFOS CORE",0Ah
    DB "4 - MON: STD HARDWARE, MONITOR-2 CORE",0Ah
    DB "5 - MON:CUSTOM: STD HARDWARE, CUSTOM CORE",0Ah,"> ",0
coreInput: BLOCK MAX_PATH,0
appName: BLOCK MAX_PATH,0
txtCustom: DB 0Ah,"MONITOR FILE: ",0
cacheToken: DW 0
    END
