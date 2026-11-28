; RUN.COM v3: общий запуск COM/EXE/RKS в MX/STD/MON/RAMFOS.
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
    INCLUDE "../common/system.inc"
    INCLUDE "../io/timer-hardware.inc"
    INCLUDE "ramfos.inc"
    INCLUDE "../../bin/bios-move-switch.inc"
    INCLUDE "../../bin/bios-imports.inc"
IMAGE_FLAGS      = IMAGE_FLAG_MAIN
RUN_BODY_REQUEST = RAMFOS_CONTEXT
RUN_LOAD_STAGE   = RAMFOS_CONTEXT+48
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
    MXOS_CALL_REGS MXOS_DOS_FILE_GET_SET_DRIVE
    ld (callerDrive),a
    pop de
    xor a
    call FONT_ENGINE_ENCODING
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
    MXOS_CALL_REGS MXOS_BIOS_ARG_NEXT
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
    xor a
    ld (monitorGiven),a
    inc hl
    inc hl
    inc hl
    ld a,(hl)
    cp ':'
    jp nz,endOption
    inc hl
    call coreName
    jp c,argumentError
    ld a,1
    ld (monitorGiven),a
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
    MXOS_CALL_REGS MXOS_DOS_FILE_NAME_PREPARE
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
    ld hl,descriptor+FILE_DESCRIPTOR.ext
    ld de,monExtension
    ld c,3
    call option
    jp z,monitorOpen
    ld a,(descriptor+FILE_DESCRIPTOR.ext)
    and 0DFh
    cp 'R'
    jp z,rksOpen
    cp 'C'
    jp z,nativeOpen
    cp 'E'
    jp nz,badImage
    jp nativeOpen
    INCLUDE "monitor.inc"
nativeOpen:
    ld hl,descriptor
    MXOS_CALL_REGS MXOS_DOS_FILE_LOAD_INFO
    jp c,badImage
    call runFileOpen
    jp c,badImage
    ld hl,rksHeader
    ld bc,IMAGE_HEADER_SIZE
    call rksRead
    jp c,badImage
    call runNativeHeader
    jp c,badImage
    ld a,(rksHeader+IMAGE_MONITOR_BYTES)
    or a
    call nz,readNamedMonitor
    jp c,badImage
    call runFileClose
    jp c,badImage
    ld a,(mode)
    inc a
    jp nz,nativeModeReady
    ld a,(rksHeader+IMAGE_MODE)
    cp IMAGE_MODE_STD
    jp nc,nativeModeFromHeader
    ld a,1
nativeModeFromHeader:
    dec a
    ld (mode),a
nativeModeReady:
    ld a,(mode)
    cp 2
    jp nz,nativeDispatch
    ld a,(monitorGiven)
    or a
    jp nz,nativeDispatch
    ld a,(rksHeader+IMAGE_MONITOR_BYTES)
    or a
    jp z,nativeDispatch
    ld hl,monitorTrailer
    call coreName
    jp c,badImage
    ld a,1
    ld (monitorGiven),a
nativeDispatch:
    ld a,(rksHeader+5)
    or a
    jp z,nativeLoadLegacy
    ld a,(mode)
    or a
    jp nz,nativeLegacyAllowed
    ld a,(rksHeader+IMAGE_MODE)
    cp IMAGE_MODE_STD
    jp c,nativeForward
nativeLegacyAllowed:
    ld a,(rksHeader+5)
    cp IMAGE_FLAG_MAIN
    jp nz,usage
nativeLoadLegacy:
    ld hl,(rksHeader+12)
    ld (expectedChecksum),hl
    jp ready
; Подтверждённый MAIN/DRIVER/BANK исполняет DOS без CLI/PATH/ассоциаций.
; main-приложение может затереть RUN: продолжение заранее переносится в 8C00h.
nativeForward:
    ld a,BIOS_PROFILE_RAMFOS
    MXOS_CALL_REGS MXOS_BIOS_PROFILE
    jp c,badImage
    ld a,(launchKoi)
    call FONT_ENGINE_ENCODING
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
    ; Пакет живёт на служебном стеке, а строку DOS читает до загрузки тела.
    ld hl,coreInput
    push af
    push bc
    push de
    push hl
    ld hl,0
    add hl,sp
    ex de,hl
    ld bc,MXOS_DOS_FILE_EXEC_NATIVE
    ld hl,biosCliExecReturn
    push hl
    jp mx_callPacket

;----------------------------------------------------------------------------
; MXOS
;
; Перенести аргумент программы в строку банкового переходника.
;
; вход:
;   hl = строка с завершающим нулём, de = свободное место назначения, b = ненулевой
;   лимит.
;
; выход:
;   При cf = 0 hl указывает на исходный ноль, de — на место для следующего байта. Ноль
;   не копируется. При исчерпании b выставлен cf, скопированный префикс остаётся в
;   назначении.
;----------------------------------------------------------------------------

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
rksOpen:
    ld hl,descriptor
    MXOS_CALL_REGS MXOS_DOS_FILE_LOAD_INFO
    jp c,notFound
    ld hl,(descriptor+FILE_DESCRIPTOR.size+2)
    ld a,h
    or l
    jp nz,badImage
    ld hl,(descriptor+FILE_DESCRIPTOR.size)
    ld (fileSize),hl
    ld de,6
    call mon2_cmp_hl_de
    jp c,badImage
    call runFileOpen
    jp c,badImage
    ld hl,rksHeader
    ld bc,4
    call rksRead
    jp c,badImage
    ld hl,(rksHeader)
    ex de,hl
    ld hl,(rksHeader+2)
    PUT_DE appLoad
    PUT_DE appEntry
    call runMath.sub_hl_de
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
    call mon2_cmp_hl_de
    jp nz,badImage
    call runFileClose
    jp c,badImage
    ld a,(mode)
    inc a
    jp nz,ready
    xor a
    ld (mode),a
ready:
    ld hl,0
    ld bc,APP_MEMORY_END
    ld a,MEM_OP_MAINCLAIM
    MXOS_CALL_REGS MXOS_DOS_MEMORY
    jp c,badImage
upperSource = ready ; Метка прежнего CPU-стенда; верхнего размещения больше нет.
    ld hl,(appLoad)
    GET_DE appSize
    add hl,de
    jp c,badImage
    ld a,(descriptor+FILE_DESCRIPTOR.ext)
    and 0DFh
    cp 'R'
    jp z,runMemoryRange
    GET_DE rksHeader+IMAGE_MEMORY_END
    call mon2_cmp_hl_de
    jp z,runMemoryEndReady
    jp nc,badImage
runMemoryEndReady:
    ex de,hl
    GET_DE rksHeader+IMAGE_MIN_HEAP
    add hl,de
    jp c,badImage
runMemoryRange:
    ld (appMemoryEnd),hl
    ld a,(mode)
    cp 3
    jp z,rfRange
    ld de,RAMFOS_BRIDGE
    ld a,(mode)
    cp 2 ; MON replaces the kernel; MX and STD keep its resident resources.
    jp z,rangeLowReady
    push hl
    call mx_getMemTop
    ld de,RAMFOS_BRIDGE-1
    call mon2_cmp_hl_de
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
    ld de,RAMFOS_BRIDGE
    ld a,(appLoad+1)
    cp 0D3h
    jp c,rangeEnd
    ld de,RAMFOS_HIGH_ADDR
rangeEnd:
    call mon2_cmp_hl_de
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
    MXOS_CALL_REGS MXOS_BIOS_GLYPH_CACHE
    jp c,badImage
    ex de,hl
    ld (cacheToken),hl
cacheRangeReady:
    ; Сохранить описание приложения до поиска сменного ядра.
    ld e,2
    MXOS_CALL_REGS MXOS_DOS_FILE_GET_SET_DRIVE
    ld (appDrive),a
    ld hl,descriptor
    ld de,bodyRequest
    ld bc,FILE_DESCR_SIZE
    call copyBytes
    ld hl,(appLoad)
    ld (bodyRequest+FILE_BODY_IO.buffer),hl
    ld a,IMAGE_HEADER_SIZE
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
    jp z,rksBodyFormat
    ld a,(rksHeader+IMAGE_MONITOR_BYTES)
    or a
    jp z,bodyPrepared
    add a,2
    ld (bodyRequest+FILE_BODY_IO.tail),a
    jp bodyPrepared
rksBodyFormat:
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
    ld hl,rfPath
    cp 3
    jp z,openCore
    ld hl,coreDefaultPath
    ld a,(monitorGiven)
    or a
    jp z,openCore
    ld hl,corePath
openCore:
    ld (coreFilePath),hl
    ld de,descriptor
    MXOS_CALL_REGS MXOS_DOS_FILE_NAME_PREPARE
    jp c,notFound
    ld hl,descriptor
    MXOS_CALL_REGS MXOS_DOS_FILE_LOAD_INFO
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
    call mon2_cmp_hl_de
    jp nz,badImage
    xor a
    ld (IO_COLOR),a
    call mon2_clearScreenOld
    ld hl,(coreFilePath)
    call readCoreToScreen
    jp c,notFound
    ld a,(mode)
    cp 3
    jp z,validateClassicRF
    ; Два известных пролога Monitor-2: оригинальный JMP и старый POP B.
    ld a,(RAMFOS_TEMP+0800h)
    cp 0C3h
    jp nz,badImage
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
    ld a,d
    cp 0C0h
    jp c,badImage
    cp 0D0h
    jp nc,badImage
    ld hl,(monSP)
    ld a,h
    cp 08Eh
    jp nc,badImage
    or a
    jp z,badImage
    GET_DE appLoad
    call mon2_cmp_hl_de
    jp c,install
    jp z,install
    ld de,64
    call runMath.sub_hl_de
    GET_DE appMemoryEnd
    call mon2_cmp_hl_de
    jp c,badImage
    jp install
validateClassicRF:
    ld a,(RAMFOS_TEMP+0800h)
    cp 0C3h
    jp nz,badImage
    ld a,(RAMFOS_TEMP+RAMFOS_CORE_SIZE+RAMFOS_START-RAMFOS_HIGH_ADDR)
    cp 0C3h
    jp nz,badImage
install:
    ; Исходный том восстанавливается до последнего копирования тела/ядра.
    ld a,(appDrive)
    ld e,1
    MXOS_CALL_REGS MXOS_DOS_FILE_GET_SET_DRIVE
    jp c,notFound
    ld a,(callerDrive)
    ld (bodyCallerDrive),a
    ; Кодировка независима от MX/STD; сменные ядра задают её сами.
    ld a,(launchKoi)
    call FONT_ENGINE_ENCODING
    ; Переходник остаётся доступным после переключения в STD.
    ld hl,RAMFOS_BRIDGE
    ld de,RAMFOS_ARGS
    ld a,(mode)
    cp 2
    jp nc,bridgeArgsReady
    ld de,MAIN_COMMAND_ADDR
bridgeArgsReady:
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
    ld (rfAfterInitEntry+1),hl
    ld hl,(callerSP)
    ld (bridgeGameSP+1-bridge+bridgeImage),hl
    ld hl,(monSP)
    ld (monAfterInitSP+1),hl
    ld hl,(monInit)
    ld (bridgeInit+1-bridge+bridgeImage),hl
    ld a,(mode)
    ld (bridgeMode-bridge+bridgeImage),a
    ld hl,(argAddress)
    ld (bridgeArguments+1-bridge+bridgeImage),hl
    ld (monAfterInitArgs+1),hl
    ld (rfAfterInitArgs+1),hl
    ; Все внутренние адреса переходника корректируются один раз при установке.
    ld hl,(bridgeBase)
    ld de,bridge
    call runMath.sub_hl_de
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
    ld hl,foreignBridge
    ld de,RAMFOS_BRIDGE
    ld bc,foreignBridgeEnd-foreignBridge
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

;----------------------------------------------------------------------------
; MXOS
;
; Открыть приложение на чтение и запомнить его каталог для загрузки тела.
;
; вход:
;   appName содержит путь с нулём, descriptor = подготовленный дескриптор. runHandle
;   доступен для записи.
;
; выход:
;   При успехе runHandle открыт, родительский каталог записан в bodyRequest, a = 0, cf
;   = 0. При ошибке открытия возвращается статус DOS. bc, de и hl рабочие.
;----------------------------------------------------------------------------

runFileOpen:
    ld hl,appName
    call runOpenPath
    ret c
    ld a,DIR_OP_RESOLVED
    MXOS_CALL_REGS MXOS_DOS_DIRECTORY
    PUT_DE bodyRequest+FILE_BODY_IO.parent
    xor a
    ret

runOpenPath:
    push hl
    ld hl,runHandle
    ld b,FILE_HANDLE
    xor a
runHandleClear:
    ld (hl),a
    inc hl
    dec b
    jp nz,runHandleClear
    pop hl
    ld (runHandle+FH_BUFFER),hl
    ld a,FILE_MODE_READ
    ld (runHandle+FH_MODE),a
    ld a,FILE_OP_OPEN_PATH
    ld hl,runHandle
    MXOS_CALL_REGS MXOS_DOS_FILEHANDLE
    ret

; Прочитать MON/RF в MAIN и скопировать в экран.
readCoreToScreen:
    call runOpenPath
    ret c
    ld hl,(descriptor+FILE_DESCRIPTOR.size)
    ld (coreRemaining),hl
    ld hl,RAMFOS_TEMP
    ld (coreScreenCursor),hl
coreScreenRead:
    ld hl,(coreRemaining)
    ld a,h
    or a
    jp z,coreScreenCount
    ld hl,0100h
coreScreenCount:
    ld b,h
    ld c,l
    push bc
    ld hl,coreReadBuffer
    call rksRead
    pop de
    ret c
    ld h,b
    ld l,c
    call mon2_cmp_hl_de
    jp nz,runHeaderBad
    push de
    ld hl,coreReadBuffer
    GET_DE coreScreenCursor
    call copyBytes
    PUT_DE coreScreenCursor
    pop de
    ld hl,(coreRemaining)
    ld a,l
    sub e
    ld l,a
    ld a,h
    sbc a,d
    ld h,a
    ld (coreRemaining),hl
    or l
    jp nz,coreScreenRead
    jp runFileClose

coreFilePath: DW 0
coreRemaining: DW 0
coreScreenCursor: DW 0
coreReadBuffer: DS 0100h,0

;----------------------------------------------------------------------------
; MXOS
;
; Прочитать часть файла RKS через открытый дескриптор.
;
; вход:
;   hl = буфер назначения, bc = запрошенная длина, runHandle открыт на чтение.
;
; выход:
;   При cf = 0 bc = фактическая длина, hl указывает за прочитанными данными. Короткое
;   чтение проверяет вызывающий. При cf = 1 возвращается ошибка DOS.
;----------------------------------------------------------------------------

runFileClose:
    ld hl,runHandle
    ld a,FILE_OP_CLOSE
    MXOS_CALL_REGS MXOS_DOS_FILEHANDLE
    ret

rksRead:
    ld (runHandle+FH_BUFFER),hl
    ld h,b
    ld l,c
    ld (runHandle+FH_COUNT),hl
    ld hl,runHandle
    ld a,FILE_OP_READ
    MXOS_CALL_REGS MXOS_DOS_FILEHANDLE
    ret c
    ld hl,(runHandle+FH_COUNT)
    ld b,h
    ld c,l
    ld hl,(runHandle+FH_BUFFER)
    add hl,bc
    or a
    ret
runNativeHeader:
    ld hl,rksHeader
    ld de,runImageMagic
    ld b,5
runMagicLoop:
    ld a,(de)
    cp (hl)
    jp nz,runHeaderBad
    inc de
    inc hl
    dec b
    jp nz,runMagicLoop
    ld a,(hl)
    cp IMAGE_FLAG_MAIN
    jp z,runHeaderFlagsReady
    cp IMAGE_FLAG_BANK+1
    jp nc,runHeaderBad
runHeaderFlagsReady:
    ld a,(rksHeader+IMAGE_HEADER_BYTES)
    cp IMAGE_HEADER_SIZE
    jp nz,runHeaderBad
    ld a,(rksHeader+IMAGE_MONITOR_BYTES)
    or a
    jp z,runHeaderMode
    cp IMAGE_MONITOR_MAX+1
    jp nc,runHeaderBad
    ld a,(rksHeader+IMAGE_MODE)
    cp IMAGE_MODE_MON
    jp nz,runHeaderBad
    ld a,(rksHeader+5)
    or a
    jp z,runHeaderMode
    cp IMAGE_FLAG_MAIN
    jp nz,runHeaderBad
runHeaderMode:
    ld a,(rksHeader+IMAGE_MODE)
    cp 5
    jp nc,runHeaderBad
    ld hl,rksHeader
    ld de,rksHeader+IMAGE_HEADER_CHECKSUM-1
    call bios_calcCS
    ld hl,(rksHeader+IMAGE_HEADER_CHECKSUM)
    ld d,b
    ld e,c
    call mon2_cmp_hl_de
    jp nz,runHeaderBad
    ld hl,(descriptor+FILE_DESCRIPTOR.size+2)
    ld a,h
    or l
    jp nz,runHeaderBad
    ld hl,(rksHeader+10)
    ld a,h
    or l
    jp z,runHeaderBad
    ld (appSize),hl
    ld de,IMAGE_HEADER_SIZE
    ld a,(rksHeader+IMAGE_MONITOR_BYTES)
    or a
    jp z,runHeaderSize
    add a,IMAGE_HEADER_SIZE+2
    ld e,a
runHeaderSize:
    add hl,de
    jp c,runHeaderBad
    GET_DE descriptor+FILE_DESCRIPTOR.size
    call mon2_cmp_hl_de
    jp nz,runHeaderBad
    ld hl,(rksHeader+6)
    ld (appLoad),hl
    ld hl,(rksHeader+8)
    ld (appEntry),hl
    GET_DE appLoad
    call mon2_cmp_hl_de
    jp c,runHeaderBad
    ld hl,(appLoad)
    GET_DE appSize
    add hl,de
    jp c,runHeaderBad
    GET_DE appEntry
    call mon2_cmp_hl_de
    jp c,runHeaderBad
    jp z,runHeaderBad
    xor a
    ret
runHeaderBad:
    scf
    ret
;----------------------------------------------------------------------------
; MXOS
;
; Прочитать и проверить назначенный монитор до чтения тела.
; вход: runHandle открыт, IMAGE_MONITOR_BYTES > 0, appSize проверен.
; выход: CF=0 имя ASCIIZ в monitorTrailer, иначе CF=1. AF/BC/DE/HL рабочие.
;----------------------------------------------------------------------------
readNamedMonitor:
    ld hl,(appSize)
    ld de,IMAGE_HEADER_SIZE
    add hl,de
    ld (runHandle+FH_SEEK),hl
    ld hl,0
    ld (runHandle+FH_SEEK+2),hl
    xor a
    ld (runHandle+FH_ORIGIN),a
    ld hl,runHandle
    ld a,FILE_OP_SEEK
    MXOS_CALL_REGS MXOS_DOS_FILEHANDLE
    ret c
    ld a,(rksHeader+IMAGE_MONITOR_BYTES)
    add a,2
    ld b,0
    ld c,a
    ld hl,monitorTrailer
    call rksRead
    ret c
    ld a,(rksHeader+IMAGE_MONITOR_BYTES)
    add a,2
    cp c
    jp nz,runHeaderBad
    ld hl,monitorTrailer
    dec c
    dec c
    add hl,bc
    push hl
    dec hl
    ex de,hl
    ld hl,monitorTrailer
    call bios_calcCS
    pop hl
    ld e,(hl)
    ld (hl),0
    inc hl
    ld d,(hl)
    ld h,b
    ld l,c
    call mon2_cmp_hl_de
    jp nz,runHeaderBad
    ld hl,monitorTrailer
    ld a,(rksHeader+IMAGE_MONITOR_BYTES)
    ld b,a
namedMonitorChar:
    ld a,(hl)
    cp 021h
    jp c,runHeaderBad
    cp 07Fh
    jp nc,runHeaderBad
    inc hl
    dec b
    jp nz,namedMonitorChar
    xor a
    ret
monitorTrailer: DS IMAGE_MONITOR_MAX+2,0
runImageMagic:
    MXOS_IMAGE_SIGNATURE
runHandle: DS FILE_HANDLE,0
; BC=вычисленная сумма, HL=два байта суммы из RKS; CF=несовпадение.
rksChecksum:
    ld e,(hl)
    inc hl
    ld d,(hl)
    ld h,b
    ld l,c
    call mon2_cmp_hl_de
    ret z
    scf
    ret
foreignBridge:
; Monitor-2 может очистить и экранный код, и весь экранный стек.
; После его инициализации продолжаем из 8C00h, затем ставим стек игры.
monAfterInitSP:
    ld sp,0
    ld hl,0C800h
    push hl
monAfterInitArgs:
    ld de,0
monAfterInitEntry:
    ld hl,0
    jp (hl)

; Инициализация RAMFOS очищает экран. Продолжение и стек вне экрана.
rfAfterCopy:
    ld sp,RAMFOS_CORE_ADDR+RAMFOS_CORE_SIZE-1
    call 0C800h
    ld hl,08FFFh
    call 0C833h
    ld hl,RAMFOS_MONITOR
    push hl
rfAfterInitArgs:
    ld de,0
rfAfterInitEntry:
    ld hl,0
    jp (hl)
foreignBridgeEnd:
    ASSERT foreignBridgeEnd-foreignBridge <= RAMFOS_ARGS-RAMFOS_BRIDGE

;----------------------------------------------------------------------------
; MXOS
;
; Сравнить ключ RUN с именем без учёта регистра.
;
; вход:
;   hl и de указывают на сравниваемые строки, c задаёт число символов для bios_stricmp.
;
; выход:
;   z = 1 при совпадении, hl сохранён. Для остальных регистров действует контракт
;   bios_stricmp.
;----------------------------------------------------------------------------

option:
    push hl
    MXOS_CALL_REGS MXOS_BIOS_STRICMP
    pop hl
    ret
; HL — имя до пробела/терминатора, сохраняется позиция разделителя.

;----------------------------------------------------------------------------
; MXOS
;
; Сформировать путь сменного ядра по аргументу команды RUN.
;
; вход:
;   hl указывает на первый символ имени. Завершает имя пробел или байт ниже 20h.
;   Рабочий буфер corePath вмещает MAX_PATH байтов.
;
; выход:
;   При cf = 0 в corePath находится строка с нулём, hl указывает на разделитель. При
;   отсутствии явных диска и расширения подставляются C:\DOS и суффикс monSuffix
;   соответственно. При пустом или слишком длинном имени cf = 1.
;----------------------------------------------------------------------------

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
    push hl
    ld hl,monSystemDirectory
    ld bc,monSystemDirectoryEnd-monSystemDirectory
    call copyBytes
    pop hl
    ld b,MAX_PATH-1-(monSystemDirectoryEnd-monSystemDirectory)
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

;----------------------------------------------------------------------------
; MXOS
;
; Последовательно скопировать блок для загрузчика RUN.
;
; вход:
;   hl = источник, de = назначение, bc = ненулевая длина. Запись вперёд не должна
;   уничтожать ещё не прочитанные байты.
;
; выход:
;   hl и de продвинуты на длину, bc = 0, a = 0, z = 1, cf = 0.
;----------------------------------------------------------------------------

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
    ld a,(runHandle+FH_STATE)
    or a
    call nz,runFileClose
    ld a,(callerDrive)
    ld e,1
    MXOS_CALL_REGS MXOS_DOS_FILE_GET_SET_DRIVE
    ld hl,(cacheToken)
    ld a,h
    or l
    jp z,errorCacheReady
    ld a,1
    MXOS_CALL_REGS MXOS_BIOS_GLYPH_CACHE
errorCacheReady:
    pop hl
    ld a,COLOR_BIOS
    ld (IO_COLOR),a
    call mon2_printStringOld
    ld a,(callerKoi)
    call FONT_ENGINE_ENCODING
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
    jp RAMFOS_BRIDGE+rfAfterCopy-foreignBridge
bridgeNative:
    inc a
    MXOS_CALL_REGS MXOS_BIOS_PROFILE
    jp c,mon2_init
bridgeReloc9:
    ld a,(bridgeMode)
    or a
bridgeReloc12:
    jp z,bridgeStart
    ld a,0FFh
    call FONT_ENGINE_ENCODING
    call biosMoveUseLow
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
    ld hl,RAMFOS_BRIDGE+monAfterInitSP-foreignBridge
    push hl
bridgeInit:
    jp 0C444h
bridgeStart:
bridgeGameSP:
    ld sp,0
    ld hl,mon2_init
    push hl
bridgeEntry:
    ld hl,0
bridgeArguments:
    ld de,RAMFOS_ARGS
    jp (hl)
; Неперекрывающиеся диапазоны: тело 0000h -> верхнее ОЗУ,
; ядро 9000h -> C000h/F800h. BC>0; AF/BC/DE/HL разрушаются.
; Свой копировальщик, без вызовов уже заменяемого DOS.
; Полный блок: 135 тактов/4 байта, четыре чтения и четыре записи.

;----------------------------------------------------------------------------
; MXOS
;
; Перенести тело программы или сменное ядро при переключении среды RUN.
;
; вход:
;   hl = начало источника, de = начало назначения, bc = ненулевой размер в байтах.
;   Диапазоны не перекрываются, код переходника доступен на всём протяжении переноса.
;
; выход:
;   Байты скопированы в порядке возрастания адресов без обращения к DOS. af, bc, de и
;   hl рабочие.
;----------------------------------------------------------------------------

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
    DB bridgeReloc9+1-bridge
    DB bridgeReloc12+1-bridge
    DB bridgeReloc10+1-bridge
    DB bridgeReloc11+1-bridge
    DB bridgeReloc16+1-bridge
    DB bridgeReloc17+1-bridge
    DB bridgeReloc18+1-bridge
bridgeRelocationsEnd:

keyMX:          DB "MX"
keyRF:          DB "RF"
keySTD:         DB "STD"
keyMON:         DB "MON"
keyKOI7:        DB "KOI7"
monSystemDirectory: DB 'A'+SYSTEM_DRIVE,':',05Ch,"DOS",05Ch
monSystemDirectoryEnd:
monSuffix:      DB ".MON"
monExtension:   DB "MON"
rfPath:         DB "C:",05Ch,"DOS",05Ch,"RAMFOS.MON",0
coreDefaultPath: DB "C:",05Ch,"DOS",05Ch,"MONITOR2.MON",0
corePath:        BLOCK MAX_PATH,0
monitorGiven:    DB 0
appArgs:        BLOCK  128,0
mode:           DB     0FFh
callerSP:       DW     0
appLoad:        DW     0
appEntry:       DW     0
appSize:        DW     0
appMemoryEnd:   DW     0

rksHeader:      BLOCK  IMAGE_HEADER_SIZE,0
bridgeBase:     DW     0
bridgeDelta:    DW     0
argAddress:     DW     0
staged:         DB     0
callerDrive:    DB     0
callerKoi:      DB     0
launchKoi:      DB     0
fileSize:       DW     0
appDrive:       DB     0
monSP:          DW     07FFFh
monInit:        DW     0C444h
descriptor:     BLOCK  FILE_DESCR_SIZE,0
txtUsage:       DB     0Ah,"RUN 3: FILE [-MX|-STD|-MON[:FILE]|-RF] [-KOI7]",0
txtNotFound:    DB     0Ah,"FILE NOT FOUND",0
txtBad:         DB     0Ah,"BAD IMAGE OR ADDRESS RANGE",0
upperEnd: ; Граница прежнего CPU-стенда, все эти байты теперь в нижнем RUN.
    MODULE runMath
    DEFINE MATH_SUB
    INCLUDE "../../sdk/asm/lib/math.inc"
    ENDMODULE
bodyRequest:        FILE_BODY_IO
expectedChecksum:   DW 0
bodyCallerDrive:    DB 0
bodyRequestEnd:
    ASSERT bodyRequestEnd-bodyRequest <= RUN_LOAD_STAGE-RUN_BODY_REQUEST

; Последний этап только читает тело, проверяет сумму и входит в переходник.
; Ошибка после затирания RUN требует bootstrap: он восстановит BIOS/шрифт.
; Адреса внутри блока отсутствуют, кроме патча последнего JP.
loadStage:
    ld hl,RUN_BODY_REQUEST
    MXOS_CALL_REGS MXOS_BIOS_FILE_BODY
    jp c,RUN_LOAD_STAGE+loadStageError-loadStage
    ld a,(RUN_BODY_REQUEST+FILE_BODY_IO+2)
    ld e,1
    MXOS_CALL_REGS MXOS_DOS_FILE_GET_SET_DRIVE
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
    call mon2_cmp_hl_de
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

coreInput:      BLOCK  MAX_PATH,0
appName:        BLOCK  MAX_PATH,0
cacheToken:     DW     0
    END
