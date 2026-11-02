;----------------------------------------------------------------------------
; MXOS: запуск EXE/RKS в MX/STD/MON; RAMFOS (-RF) делегируется RAMRUN.
; Вход: DE — аргументы. Выход до передачи: CF=1 ошибка, регистры разрушены.
; В STD код F800h исчезает; переход и возврат выполняются из E800h.
;----------------------------------------------------------------------------
    INCLUDE "../include/mxos.inc"
    ORG 0F800h

start:
    ld hl,0
    add hl,sp
    ld (callerSP),hl
    ex de,hl
    call spaces
    ld a,2                     ; прежний запуск без ключа означает MON
    ld (mode),a
    ld a,(hl)
    cp '-'
    jp nz,loadGame
    inc hl
    ld de,txtMX
    ld c,2
    call option
    jp z,modeMX
    ld de,txtRF
    call option
    jp z,launchRF
    ld de,txtSTD
    ld c,3
    call option
    jp z,modeSTD
    ld de,txtMON
    call option
    jp nz,usage
    inc hl
    inc hl
    inc hl
    ld a,(hl)
    cp ':'
    jp nz,endOption
    inc hl
    ld de,monitorPath+2
    ld b,8
monitorName:
    ld a,(hl)
    cp ' '+1
    jp c,monitorSuffix
    cp '.'
    jp z,usage
    cp ':'
    jp z,usage
    ld (de),a
    inc de
    inc hl
    dec b
    jp nz,monitorName
    ld a,(hl)
    cp ' '+1
    jp nc,usage
monitorSuffix:
    ld a,b
    cp 8
    jp z,usage
    push hl
    ld hl,txtSuffix
    ld bc,5
    call copyBytes
    pop hl
    jp endOption
; HL указывает на RF. Команда во временной области NC переживает загрузку
; RAMRUN поверх F800h. bios_fileExec получает строку и сам передаёт аргументы.
; Вход: HL — ключ RF; выход через RAMRUN/DOS, AF/BC/DE/HL разрушены.
launchRF:
    inc hl
    inc hl
    ld a,(hl)
    cp ' '
    jp nz,usage
    call spaces
    ld a,(hl)
    cp ' '+1
    jp c,usage
    push hl
    ld hl,txtRamRun
    ld de,bridge
    ld bc,13
    call copyBytes
    pop hl
copyRFArgs:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    cp ' '
    jp nc,copyRFArgs
    ld hl,bridge
    jp bios_fileExec
modeMX:
    xor a
    ld (mode),a
    inc hl
    inc hl
    jp endOption
modeSTD:
    ld a,1
    ld (mode),a
    inc hl
    inc hl
    inc hl
endOption:
    ld a,(hl)
    cp ' '
    jp nz,usage
    call spaces
loadGame:
    ld a,(hl)
    cp ' '+1
    jp c,usage
    ld de,descriptor
    call bios_fileNamePrepare
    ld (appArgs),hl
    ld hl,descriptor
    call bios_fileLoadInfo
    jp c,notFound
    ; Нельзя загрузить поверх стека, ядра или самого LAUNCH.
    ld hl,(descriptor+FILE_DESCRIPTOR.size+2)
    ld a,h
    or l
    jp nz,badImage
    ld hl,(descriptor+FILE_DESCRIPTOR.loadAddress)
    ld (appEntry),hl
    ex de,hl
    ; fileLoad читает полные секторы: проверяется именно записываемый диапазон.
    ld a,(descriptor+FILE_DESCRIPTOR.size+1)
    inc a
    jp z,badImage
    ld h,a
    ld l,0
    add hl,de
    jp c,badImage
    ld de,bios_vars             ; обычная нагрузка только ниже переменных BIOS
    call bios_cmp_hl_de
    jp c,gameRangeOK
    jp nz,badImage
gameRangeOK:
    ld hl,descriptor
    call bios_fileLoad
    jp c,notFound
    ld a,(mode)
    or a
    jp nz,prepareSTD
    ld hl,(appArgs)
    ex de,hl
    ld hl,(appEntry)
    jp (hl)                    ; MX: тот же стек возврата, что у EXE

prepareSTD:
    ld a,(bios_vars.koi7)
    ld (bridgeKoi7-bridge+bridgeImage),a
    ; Общая подготовка STD/MON: все каналы ВИ53 остановлены без счёта.
    ld hl,IO_TIMER_MODE
    ld (hl),0B6h
    ld (hl),076h
    ld (hl),036h
    ld a,00Ah
    ld (IO_KEYB_MODE),a
    ld hl,(callerSP)
    ld (bridgeSP+1-bridge+bridgeImage),hl
    ld (bridgeCallerSP+1-bridge+bridgeImage),hl
    ld hl,(appEntry)
    ld (bridgeEntry+1-bridge+bridgeImage),hl
    ld hl,(appArgs)
    ld (bridgeArgs+1-bridge+bridgeImage),hl
    ld a,(mode)
    ld (bridgeMode-bridge+bridgeImage),a
    dec a
    jp z,installBridge

    ld hl,monitorPath
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
    ld de,0FFFh                ; известный 4-КБ формат Monitor-2/Loader
    call bios_cmp_hl_de
    jp nz,badImage
    ld hl,descriptor
    ld de,0E000h
    call bios_fileLoad2
    jp c,notFound
    ; Инициализация — через адаптер, не через патченный старт монитора.
    ; Два известных пролога: исходный JMP C003/LXI SP и старый POP BC.
    ld a,(0E000h)
    cp 0C1h
    jp z,legacyMonitor
    cp 0C3h
    jp nz,badImage
    ld a,(0E003h)
    cp 031h
    jp nz,badImage
    ld hl,(0E004h)
    ld de,0E00Bh
    jp monitorInit
legacyMonitor:
    ld a,(0E004h)
    cp 031h
    jp nz,badImage
    ld hl,(0E005h)
    ld de,0E00Dh
monitorInit:
    ld (bridgeSP+1-bridge+bridgeImage),hl
    ld a,(de)
    cp 0C3h
    jp nz,badImage
    inc de
    ld a,(de)
    ld l,a
    inc de
    ld a,(de)
    cp 0C0h
    jp c,badImage
    cp 0D0h
    jp nc,badImage
    ld h,a
    ld (bridgeTransfer+2-bridge+bridgeImage),hl
    ld hl,0C3E5h               ; PUSH H; JMP init
    ld (bridgeTransfer-bridge+bridgeImage),hl
    ld hl,0E000h
    ld de,0C000h
    ld bc,1000h
    call copyBytes             ; после этого старый ABI вызывать нельзя
installBridge:
    ld hl,bridgeImage
    ld de,bridge
    ld bc,bridgeEnd-bridge
    call copyBytes
    jp bridge

; Вход HL — строка. Выход HL — первый непробельный байт; AF разрушен.
spaces:
    ld a,(hl)
    cp ' '
    ret nz
    inc hl
    jp spaces
; Вход HL/DE — строки, C — длина. Выход Z; HL сохранён, AF/DE разрушены.
option:
    push hl
    call bios_stricmp
    pop hl
    ret
; HL источник, DE приёмник, BC ненулевая длина. Разрушает AF/BC/DE/HL.
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
    ld hl,txtBadImage
error:
    call bios_printString
    scf
    ret

bridgeImage:
    DISP 0E800h
bridge:
bridgeSP:
    ld sp,0
    ld hl,bridgeReturn
    push hl
bridgeEntry:
    ld hl,0
bridgeArgs:
    ld de,0
    ld a,0FFh                  ; MXOS в STD печатает KOI-7 старых программ
    ld (bios_vars.koi7),a
    ld a,3
    ld (IO_PAGE_STD),a
    ; Только после смены режима: это STD-регистр RAM/ROM и 4/8 цветов,
    ; а не MX-защёлка цвета по FFF8h. Прежний Monitor-2 писал именно F7FAh.
    ld (IO_COLOR-07FEh),a       ; F7FAh: нижняя RAM видна, 8 цветов
    ld a,082h
    ld (0F803h),a
    xor a                      ; белый цвет стандартного контроллера
    ld (0F802h),a
bridgeTransfer:
    jp (hl)
    DB 0,0,0                   ; MON заменяет на PUSH H; JMP init
bridgeReturn:
    ld a,(bridgeMode)
    cp 2
    jp z,bridgeCold
    ld (IO_PAGE_RAM-800h),a
bridgeCallerSP:
    ld sp,0
    ld a,(bridgeKoi7)
    ld (bios_vars.koi7),a
    ld a,COLOR_BIOS
    ld (IO_COLOR),a
    or a
    ret
bridgeCold:
    xor a                      ; первая страница системной flash, не номер режима
    ld (IO_PAGE_ROM-800h),a
    jp 0
bridgeMode:
    DB 0
bridgeKoi7:
    DB 0
bridgeEnd:
    ENT

txtMX: DB "MX"
txtRF: DB "RF"
txtRamRun: DB "A:RAMRUN.COM "
txtSTD: DB "STD"
txtMON: DB "MON"
txtSuffix: DB ".MON",0
txtUsage: DB 0Ah,"LAUNCH -MX|-STD|-MON|-RF FILE",0
txtNotFound: DB 0Ah,"FILE NOT FOUND",0
txtBadImage: DB 0Ah,"BAD RANGE OR MONITOR",0
monitorPath: DB "A:MONITOR2.MON",0,0,0
mode: DB 2
callerSP: DW 0
appEntry: DW 0
appArgs: DW 0
descriptor: BLOCK FILE_DESCR_SIZE,0
    ASSERT_DONT_FIT 0FB00h
    END
