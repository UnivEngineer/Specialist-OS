; Холодная загрузка: BIOS в main, DOS в физическую страницу ДОЗУ 0.
; Адреса файлов фиксированы сборщиком. FAT не нужен bootstrap.
    INCLUDE "../../bin/bootstrap/config.inc"
    IFDEF ORIGINAL_MX
    INCLUDE "../../bin/mx/imports.inc"
    ASSERT BOOT_FONT_ROM+MX_ROM_FONT_BYTES <= BOOT_IO_ROM
    ENDIF
    ORG 02000h
stageImage:
    DISP BOOT_STAGE_BASE

;----------------------------------------------------------------------------
; MXOS
;
; Установить банковую среду MXOS и передать управление BIOS.
;
; вход:
;   Загрузочный сектор разместил эту ступень по BOOT_STAGE_BASE. Адреса системных
;   сегментов в ROM уже заданы конфигурацией сборки.
;
; выход:
;   Стартует BIOS по C000h в MAIN с установленным транспортом. На холодном пути
;   загружается DOS; тёплый путь определяется состоянием памяти и ревизией. Возврат в
;   сектор загрузки не предусмотрен.
;----------------------------------------------------------------------------

entry:
stage:
    di
    ld sp,08FDFh
    IFDEF ORIGINAL_MX
    ld a,0
    rla
    ld (mxBootMedium),a
    call mxPickBundle
    jp c,mxRetry
    ENDIF
    IFNDEF ORIGINAL_MX
    call drawBootLogo
logoShown:
    ENDIF
    ld hl,03000h
    ld de,03000h+DOS_BYTES
    ld bc,0C000h
    call moveCore
biosLoaded:
    ld hl,BOOT_IO_ROM
    ld de,BOOT_IO_ROM+BOOT_IO_BYTES
    ld bc,BOOT_IO_RAM
    call moveCore
ioLoaded:
    ; В слоте шрифта пропускаем заголовок DLL.
    ld hl,BOOT_FONT_ROM
    ld de,BOOT_FONT_ROM+BOOT_FONT_BYTES
    IFDEF ORIGINAL_MX
    ld a,(mxRomBundle)
    or a
    jp z,mxFontSelected
    ld de,BOOT_FONT_ROM+MX_ROM_FONT_BYTES
mxFontSelected:
    ENDIF
    ld bc,BOOT_FONT_RAM
    call moveCore
fontLoaded:
    ld hl,bootFontName
    ld de,bootFontName+8
    ld bc,BOOT_FONT_NAME_RAM
    call moveCore
    call checkBootRam
    jp z,bootFailed
    ld (pageCount),a
    ld (dosContext+1),a
    ld (BIOS_BOOT_PAGES),a
    ; Детектор оставил в общем окне байтовые входы bootstrap. До решения о
    ; тёплом старте можно читать банки без установленного транспорта MEM.
    IFDEF ORIGINAL_MX
    INCLUDE "mx-load.inc"
    INCLUDE "mx-bundle.inc"
    ELSE
    ; Проверяем неизменяемые пары развёртки CPM32. Код COMMAND меняется с ABI.
    ld hl,BOOT_RAM_SIGNATURE
    ld de,memImage+MEM_DATA_OFFSET+(BOOT_RAM_SIGNATURE-MEM_DATA_START)
    ld b,8
checkRamTail:
    ld a,1
    call BOOT_BYTE_READ
    xor a
    ld (0FFFEh),a
    ld a,(de)
    ld (0FFFCh),a
    cp c
    jp nz,ramTailChecked
    inc hl
    inc de
    dec b
    jp nz,checkRamTail
    xor a
    ld (ramFresh),a
ramTailChecked:
    ld a,(ramFresh)
    or a
    jp nz,loadDos
    ; BIOS загружается всегда. DOS сохраняем только по её собственному
    ; флагу готовности и совместимой ревизии MEM; это не проверка целостности.
    ld hl,DOS_READY
    xor a
    call BOOT_BYTE_READ
    ld a,c
    cp 1
    jp nz,loadDos
    ; При смене внутреннего ABI старое ядро нельзя соединять с новым MEM.
    ; Это проверка версии связки, не контроль целостности DOS.
    ld hl,BOOT_MEM_REVISION_ADDR
    xor a
    call BOOT_BYTE_READ
    ld a,c
    cp BOOT_MEM_REVISION
    jp nz,loadDos
    inc hl
    xor a
    call BOOT_BYTE_READ
    ld a,c
    or a
    jp nz,loadDos
    jp checkWarmPages
loadDos:
    ; Сбрасываем готовность до первой записи, в том числе при смене ревизии.
    xor a
    ld hl,DOS_READY
    ld c,a
    call BOOT_BYTE_WRITE
    ld a,1                      ; DOS.SYS начинается в следующем окне ROM.
    ld (installRomPage+1),a
    ld de,0
    ld hl,MODULE_LOAD
    ASSERT MODULE_LOAD+BOOT_KERNEL_BYTES <= BOOT_KERNEL_END_LIMIT
    IF BOOT_KERNEL_BYTES > 08000h
    ; Окно ПЗУ занимает 32 КиБ. Хвост DOS начинается с нулевого
    ; смещения следующей страницы ПЗУ, сразу за первым окном образа.
    ld bc,08000h
    xor a
    call install
    ld a,2
    ld (installRomPage+1),a
    ld de,0
    ld hl,MODULE_LOAD+08000h
    ld bc,BOOT_KERNEL_BYTES-08000h
    ELSE
    ld bc,BOOT_KERNEL_BYTES
    ENDIF
    xor a
    call install
    xor a
    ld (installRomPage+1),a
    ENDIF
dosLoaded:
    ld a,(BIOS_BOOT_WARM)
    or a
    jp nz,installDosProfile
    ld de,ramFresh
    ld hl,BOOT_RAM_FRESH
    ld bc,1
    call install
    ; Нулевой банк сразу получает полный TRANSPORT с собственным MOVE.
    ld a,(pageCount)
installPages:
    dec a
    jp z,installDosProfile
    push af
    ld de,memImage+MEM_DATA_OFFSET
    ld hl,MEM_DATA_START
    ld bc,MEM_DATA_SIZE
    call install
    pop af
    push af
    ld de,dataForwardImage
    ld hl,BOOT_DATA_FORWARD
    ld bc,BOOT_DATA_FORWARD_BYTES
    call install
    pop af
    push af
    ld de,dataForwardWord
    ld hl,BOOT_MOVE_OPERAND
    ld bc,2
    call install
    pop af
    jp installPages
installDosProfile:
    IFNDEF ORIGINAL_MX
    xor a
    ld (0FFFEh),a             ; MEM.SYS читается из собственного ROM-слота.
    ENDIF
    ld hl,transportImage
    ld de,transportEnd
    ld bc,0FC00h
    call moveCore
    ld a,(pageCount)
    ld (BANK_PAGE_COUNT),a
    ; В шаблоне MAIN уже фактическое число страниц. Повторный Reset
    ; посреди установки DOS увидит прежнюю геометрию и сохранит её ядро.
    ld de,0FC00h
    ld hl,0FC00h
    ld bc,transportEnd-transportImage
    xor a
    call install
    ld de,clientImage
    ld hl,BOOT_BANK_CLIENT
    ld bc,clientEnd-clientImage
    xor a
    call install
    ld de,BOOT_IO_ROM+BOOT_IO_BYTES
    ld hl,BOOT_IO_KEYS
    ld bc,BOOT_IO_KEY_BYTES
    xor a
    call install
    ld de,localBlockImage
    ld hl,BOOT_LOCAL_BLOCK
    ld bc,localBlockEnd-localBlockImage
    xor a
    call install
    ld de,dosKind
    ld hl,BOOT_MEM_KIND
    ld bc,1
    xor a
    call install
    ld de,dosLocalA
    ld hl,BOOT_MEM_LOCAL_SYSTEM
    ld bc,2
    xor a
    call install
    ld de,dosNearCopy
    ld hl,BOOT_MEM_NEAR_COPY
    ld bc,2
    xor a
    call install
    ld de,dosContext
    ld hl,BANK_PAGE
    ld bc,dosContextEnd-dosContext
    xor a
    call install
    IFNDEF ORIGINAL_MX
    xor a
    ld (0FFFEh),a
    ENDIF
    ld hl,BOOT_BIOS_STACK_TOP
    ld (BANK_SERVICE_SP),hl
    ld hl,BOOT_BIOS_STACK_FLOOR
    ld (BANK_STACK_FLOOR),hl
    ; MAIN пропускает слоты драйвера C: и банковых обработчиков CLIENT.
    ld hl,localBlockImage
    ld de,localBlockImage+BANK_MAIN_LOCAL_BYTES
    ld bc,BOOT_LOCAL_BLOCK
    call moveCore
    ld hl,mainClientImage
    ld de,mainClientImage+BANK_MAIN_CLIENT_BYTES
    ld bc,BOOT_MAIN_CLIENT
    call moveCore
    call installMainBiosOverlay
    ld hl,windowImage
    ld de,windowEnd
    ld bc,0FFC0h
    call moveCore
    ld a,1
    ld (BIOS_BANKED),a
    ld (BIOS_DOS_READY),a
bootDone:
    xor a
    ld (0FFFCh),a
    ld hl,bootBiosStart
    ld (biosBootEntry),hl
    jp 0C000h
bootFailed:
    halt
    jp bootFailed

    INCLUDE "bios-overlay.inc"
    INCLUDE "bios-start.inc"

;----------------------------------------------------------------------------
; MXOS
; Завершить проверку тёплого Reset перед использованием сохранённых банков.
; вход: DOS готова и совместима, в окне байтовые входы bootstrap, pageCount от детектора.
; выход: при прежней геометрии BIOS_BOOT_WARM=1 и переход к dosLoaded.
; При изменении числа банков начинается холодная загрузка. AF/C/HL изменены.
;----------------------------------------------------------------------------
checkWarmPages:
    xor a
    ld hl,BANK_PAGE_COUNT
    call BOOT_BYTE_READ
    ld a,(pageCount)
    cp c
    IFDEF ORIGINAL_MX
    jp nz,mxLoadDos
    ELSE
    jp nz,loadDos
    ENDIF
    ld a,1
    ld (BIOS_BOOT_WARM),a
    jp dosLoaded

    INCLUDE "check-ram.inc"
    INCLUDE "install.inc"
    IFNDEF ORIGINAL_MX
    INCLUDE "splash.inc"
    ENDIF
    INCLUDE "memcpy.inc"
bootFontName:    DB BOOT_SELECTED_BASENAME
pageCount:       DB 16
dataForwardWord: DW BOOT_DATA_FORWARD
ramFresh:        DB 1
dosKind:         DB 2
    IFDEF ORIGINAL_MX
dosLocalA:       DW MX_DOS_LOCAL_A
dosNearCopy:     DW MX_DOS_NEAR_COPY
    ELSE
dosLocalA:       DW BOOT_DOS_LOCAL_A
dosNearCopy:     DW BOOT_DOS_NEAR_COPY
    ENDIF
dosContext:
    DB 1
    ; Число страниц после детектора копируется сюда перед установкой.
    DB 16,0,0,0
    DW BOOT_DOS_STACK_TOP,BOOT_DOS_STACK_BOTTOM+64
dosContextEnd:
memImage        = BOOT_MEM_ROM
transportImage  = memImage+MEM_TRANSPORT_OFFSET
transportEnd    = transportImage+MEM_TRANSPORT_SIZE
clientImage     = memImage+MEM_CLIENT_OFFSET
clientEnd       = clientImage+MEM_CLIENT_SIZE
mainClientImage = clientImage+BOOT_MAIN_CLIENT-BOOT_BANK_CLIENT
localBlockImage = memImage+MEM_LOCAL_OFFSET
localBlockEnd   = localBlockImage+MEM_LOCAL_SIZE
windowImage     = memImage+MEM_WINDOW_OFFSET
windowEnd       = windowImage+MEM_WINDOW_SIZE
    ENT
dataForwardStorage:
    DISP BOOT_DATA_FORWARD
    INCLUDE "../mem/data-forward.inc"
    ENT
dataForwardStorageEnd:
dataForwardImage = BOOT_STAGE_BASE+dataForwardStorage-stageImage
    ASSERT dataForwardStorageEnd-dataForwardStorage == BOOT_DATA_FORWARD_BYTES
stageEnd:
    ASSERT stageEnd <= 03000h
    ; Холодный bootstrap временно занимает также свободную область RUN.
    ; Стек и переменные BIOS остаются выше BOOT_STAGE_LIMIT.
    ASSERT BOOT_STAGE_BASE+stageEnd-stageImage <= BOOT_STAGE_LIMIT
