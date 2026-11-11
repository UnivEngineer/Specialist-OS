; Холодная загрузка: BIOS в main, DOS в физическую страницу ДОЗУ 0.
; Адреса файлов фиксированы сборщиком. FAT не нужен bootstrap.
    INCLUDE "../../bin/bootstrap/config.inc"
    ORG 02000h
entry:
    di
    ld hl,stageImage
    ld de,08000h
    ld bc,stageEnd-stageImage
relocate:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,relocate
    jp stage
stageImage:
    DISP 08000h
stage:
    ld sp,08FDFh
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
    ; FONT.FNT остаётся линейным на диске; RAM — восемь страниц строк.
    ld hl,BOOT_FONT_ROM
    ld e,0
fontCode:
    ld d,BOOT_FONT_RAM >> 8
    DUP 8
    ld a,(hl)
    ld (de),a
    inc hl
    inc d
    EDUP
    inc e
    jp nz,fontCode
fontLoaded:
    ; Детектор работает до установки общего шлюза.
    ld a,15
    ld hl,0FFBEh
    call BIOS_CHECK_RAMD
    jp z,bootFailed
    ; Адрес поля берётся из структуры BIOS, не из конца области.
    ld a,(BIOS_PAGE_COUNT)
    ld (pageCount),a
    ld (dosContext+1),a
    ld (BIOS_BOOT_PAGES),a
    ld hl,byteWindow
    ld de,byteWindowEnd
    ld bc,0FFC0h
    call moveCore
    ; BIOS загружается всегда. DOS сохраняем только по её собственному
    ; флагу готовности и совместимой ревизии MEM; это не проверка целостности.
    ld hl,DOS_READY
    call 0FFC0h+byteRead-byteWindow
    ld a,c
    cp 1
    jp nz,loadDos
    ; При смене внутреннего ABI старое ядро нельзя соединять с новым MEM.
    ; Это проверка версии связки, не контроль целостности DOS.
    ld hl,BOOT_MEM_REVISION_ADDR
    call 0FFC0h+byteRead-byteWindow
    ld a,c
    cp BOOT_MEM_REVISION
    jp nz,loadDos
    inc hl
    call 0FFC0h+byteRead-byteWindow
    ld a,c
    or a
    jp nz,loadDos
    ld a,1
    ld (BIOS_BOOT_WARM),a
    jp dosLoaded
loadDos:
    ; R4 ROM -> DOS: код хвоста одинаков в обеих страницах.
    ld de,romTailImage
    ld hl,0FF00h
    ld bc,romTailEnd-romTailImage
    xor a
    call install
    ld hl,romTailImage
    ld de,romTailEnd
    ld bc,0FF00h
    call moveCore
    ld hl,romWindow
    ld de,romWindowEnd
    ld bc,0FFC0h
    call moveCore
    ld hl,0
    add hl,sp
    dec hl
    dec hl
    ld (0FFD4h),hl
    ld a,MODULE_SECTORS
    ld (0FFD3h),a
    ld hl,MODULE_LOAD+4
    call 0FFC0h
dosLoaded:
    ; Данные страниц RAM-диска не трогаем, устанавливаем только FF00h.
    ld hl,byteWindow
    ld de,byteWindowEnd
    ld bc,0FFC0h
    call moveCore
    ld a,(pageCount)
installPages:
    dec a
    push af
    ld de,memImage+MEM_DATA_OFFSET
    ld hl,0FF00h
    ld bc,MEM_DATA_SIZE
    call install
    pop af
    or a
    jp nz,installPages
    ld de,transportImage
    ld hl,0FC00h
    ld bc,transportEnd-transportImage
    xor a
    call install
    ld de,clientImage
    ld hl,0F800h
    ld bc,clientEnd-clientImage
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
    ld hl,BOOT_MEM_LOCAL_A
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
    ld hl,transportImage
    ld de,transportEnd
    ld bc,0FC00h
    call moveCore
    ld a,(pageCount)
    ld (BANK_PAGE_COUNT),a
    ld hl,BOOT_BIOS_STACK_TOP
    ld (BANK_SERVICE_SP),hl
    ld hl,BOOT_BIOS_STACK_FLOOR
    ld (BANK_STACK_FLOOR),hl
    ; Полный MEM-клиент одинаков в main и DOS; BIOS задаст локальный вход.
    ld hl,localBlockImage
    ld de,localBlockEnd
    ld bc,BOOT_LOCAL_BLOCK
    call moveCore
    ld hl,clientImage
    ; MAIN не получает локальные службы EXEC/DOS в слоте FLASHSYS.
    ld de,clientImage+BANK_MAIN_CLIENT_BYTES
    ld bc,0F800h
    call moveCore
    ld hl,windowImage
    ld de,windowEnd
    ld bc,0FFC0h
    call moveCore
    ld a,1
    ld (BIOS_BANKED),a
    ld (BIOS_DOS_READY),a
bootDone:
    jp 0C000h
bootFailed:
    halt
    jp bootFailed

; A=физическая страница, DE=источник main, HL=назначение, BC=длина.
; Общий байтовый установщик нужен только для хвостов до запуска транспорта.
install:
    ld (0FFC1h),a
installLoop:
    ld a,(de)
    push bc
    ld c,a
    call 0FFC0h
    pop bc
    inc de
    inc hl
    dec bc
    ld a,b
    or c
    jp nz,installLoop
    ret
byteWindow:
    ld a,0
    ld (0FFFDh),a
    ld (hl),c
    ld (0FFFCh),a
    ret
byteRead:
    ; Второй вход читает байт из физической страницы 0 в C.
    ld a,0
    ld (0FFFDh),a
    ld c,(hl)
    ld (0FFFCh),a
    ret
byteWindowEnd:
    ASSERT byteWindowEnd-byteWindow == 20
romWindow:
    ld sp,0
    ld a,1
    ld (0FFFEh),a
    jp romRead4
    xor a
    ld (0FFFDh),a
    jp ramWrite4
romWindowEnd:
    ASSERT romWindowEnd-romWindow == 18
    INCLUDE "memcpy.inc"
pageCount: DB 16
dosKind: DB 2
dosLocalA: DW BOOT_DOS_LOCAL_A
dosNearCopy: DW BOOT_DOS_NEAR_COPY
dosContext:
    DB 1
    ; Число страниц после детектора копируется сюда перед установкой.
    DB 16,0,0,0
    DW BOOT_DOS_STACK_TOP,BOOT_DOS_STACK_BOTTOM+64
dosContextEnd:
memImage:
    INCBIN "../../bin/MEM.sys"
transportImage = memImage+MEM_TRANSPORT_OFFSET
transportEnd = transportImage+MEM_TRANSPORT_SIZE
clientImage = memImage+MEM_CLIENT_OFFSET
clientEnd = clientImage+MEM_CLIENT_SIZE
localBlockImage = memImage+MEM_LOCAL_OFFSET
localBlockEnd = localBlockImage+MEM_LOCAL_SIZE
windowImage = memImage+MEM_WINDOW_OFFSET
windowEnd = windowImage+MEM_WINDOW_SIZE
    ENT
romTailStorage:
    DISP 0FF00h
romRead4:
    pop bc
    pop de
    jp 0FFCBh
ramWrite4:
    ld sp,hl
    push de
    push bc
    inc hl
    inc hl
    inc hl
    inc hl
    ld a,(0FFC1h)
    add a,4
    ld (0FFC1h),a
    jp nz,0FFC0h
    ld a,(0FFC2h)
    inc a
    cp 080h
    jp nz,romSamePage
    ld a,(0FFC4h)
    inc a
    ld (0FFC4h),a
    xor a
romSamePage:
    ld (0FFC2h),a
    ld a,(0FFD3h)
    dec a
    ld (0FFD3h),a
    jp nz,0FFC0h
    ld (0FFFCh),a
    ld hl,(0FFD4h)
    ld sp,hl
    ret
    ENT
romTailStorageEnd:
romTailImage = 08000h+romTailStorage-stageImage
romTailEnd = romTailImage+romTailStorageEnd-romTailStorage
stageEnd:
    ASSERT stageEnd <= 03000h
    ; Холодный bootstrap временно занимает также свободную область RUN.
    ; Стек и переменные BIOS остаются выше BOOT_STAGE_LIMIT.
    ASSERT 08000h+stageEnd-stageImage <= BOOT_STAGE_LIMIT
