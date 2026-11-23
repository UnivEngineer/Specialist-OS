; РҐРѕР»РѕРґРЅР°СЏ Р·Р°РіСЂСѓР·РєР°: BIOS РІ main, DOS РІ С„РёР·РёС‡РµСЃРєСѓСЋ СЃС‚СЂР°РЅРёС†Сѓ Р”РћР—РЈ 0.
; РђРґСЂРµСЃР° С„Р°Р№Р»РѕРІ С„РёРєСЃРёСЂРѕРІР°РЅС‹ СЃР±РѕСЂС‰РёРєРѕРј. FAT РЅРµ РЅСѓР¶РµРЅ bootstrap.
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
    ; Fixed-slot module includes its prepared font. Skip the DLL header in ROM.
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
    ; Р”РµС‚РµРєС‚РѕСЂ СЂР°Р±РѕС‚Р°РµС‚ РґРѕ СѓСЃС‚Р°РЅРѕРІРєРё РѕР±С‰РµРіРѕ С€Р»СЋР·Р°.
    ld a,15
    ld hl,0FFBEh
    call BIOS_CHECK_RAMD
    jp z,bootFailed
    ; РђРґСЂРµСЃ РїРѕР»СЏ Р±РµСЂС‘С‚СЃСЏ РёР· СЃС‚СЂСѓРєС‚СѓСЂС‹ BIOS, РЅРµ РёР· РєРѕРЅС†Р° РѕР±Р»Р°СЃС‚Рё.
    ld a,(BIOS_PAGE_COUNT)
    ld (pageCount),a
    ld (dosContext+1),a
    ld (BIOS_BOOT_PAGES),a
    ld hl,byteWindow
    ld de,byteWindowEnd
    ld bc,0FFC0h
    call moveCore
    ; Р”Рѕ РїРµСЂРµСѓСЃС‚Р°РЅРѕРІРєРё MEM РїСЂРѕРІРµСЂСЏРµРј РІРѕСЃРµРјСЊ Р±Р°Р№С‚РѕРІ РєРѕРїРёСЂРѕРІС‰РёРєР° РІ РїРµСЂРІРѕРј
    ; Р±Р°РЅРєРµ B:. РџРѕСЃР»Рµ РїРѕС‚РµСЂРё РћР—РЈ СЃРѕРґРµСЂР¶РёРјРѕРјСѓ boot-СЃРµРєС‚РѕСЂР° РґРѕРІРµСЂСЏС‚СЊ РЅРµР»СЊР·СЏ.
    IFDEF ORIGINAL_MX
    INCLUDE "mx-load.inc"
    INCLUDE "mx-bundle.inc"
    ELSE
    ld a,1
    ld (0FFC0h+byteRead-byteWindow+1),a
    ld hl,0FF04h
    ld de,memImage+MEM_DATA_OFFSET+(0FF04h-MEM_DATA_START)
    ld b,8
checkRamTail:
    call 0FFC0h+byteRead-byteWindow
    call readSource
    cp c
    jp nz,ramTailChecked
    inc hl
    inc de
    dec b
    jp nz,checkRamTail
    xor a
    ld (ramFresh),a
ramTailChecked:
    xor a
    ld (0FFC0h+byteRead-byteWindow+1),a
    ld a,(ramFresh)
    or a
    jp nz,loadDos
    ; BIOS Р·Р°РіСЂСѓР¶Р°РµС‚СЃСЏ РІСЃРµРіРґР°. DOS СЃРѕС…СЂР°РЅСЏРµРј С‚РѕР»СЊРєРѕ РїРѕ РµС‘ СЃРѕР±СЃС‚РІРµРЅРЅРѕРјСѓ
    ; С„Р»Р°РіСѓ РіРѕС‚РѕРІРЅРѕСЃС‚Рё Рё СЃРѕРІРјРµСЃС‚РёРјРѕР№ СЂРµРІРёР·РёРё MEM; СЌС‚Рѕ РЅРµ РїСЂРѕРІРµСЂРєР° С†РµР»РѕСЃС‚РЅРѕСЃС‚Рё.
    ld hl,DOS_READY
    call 0FFC0h+byteRead-byteWindow
    ld a,c
    cp 1
    jp nz,loadDos
    ; РџСЂРё СЃРјРµРЅРµ РІРЅСѓС‚СЂРµРЅРЅРµРіРѕ ABI СЃС‚Р°СЂРѕРµ СЏРґСЂРѕ РЅРµР»СЊР·СЏ СЃРѕРµРґРёРЅСЏС‚СЊ СЃ РЅРѕРІС‹Рј MEM.
    ; Р­С‚Рѕ РїСЂРѕРІРµСЂРєР° РІРµСЂСЃРёРё СЃРІСЏР·РєРё, РЅРµ РєРѕРЅС‚СЂРѕР»СЊ С†РµР»РѕСЃС‚РЅРѕСЃС‚Рё DOS.
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
    ; R4 ROM -> DOS: РєРѕРґ С…РІРѕСЃС‚Р° РѕРґРёРЅР°РєРѕРІ РІ РѕР±РµРёС… СЃС‚СЂР°РЅРёС†Р°С….
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
    ENDIF
dosLoaded:
    ; Install the current MEM tails in DOS and data banks.
    ld hl,byteWindow
    ld de,byteWindowEnd
    ld bc,0FFC0h
    call moveCore
    ld a,(BIOS_BOOT_WARM)
    or a
    jp nz,ramStateReady
    ; A freshly installed image has no initialized data at DOS_READY.
    ld hl,DOS_READY
    ld c,0
    ld (0FFC1h),a
    call 0FFC0h
    ld de,ramFresh
    ld hl,BOOT_RAM_FRESH
    ld bc,1
    xor a
    call install
ramStateReady:
    ld a,(pageCount)
installPages:
    dec a
    push af
    ld de,memImage+MEM_DATA_OFFSET
    ld hl,MEM_DATA_START
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
    ld (0FFFEh),a             ; MEM.SYS С‡РёС‚Р°РµС‚СЃСЏ РёР· СЃРѕР±СЃС‚РІРµРЅРЅРѕРіРѕ ROM-СЃР»РѕС‚Р°.
    ENDIF
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
    ; РџРѕР»РЅС‹Р№ MEM-РєР»РёРµРЅС‚ РѕРґРёРЅР°РєРѕРІ РІ main Рё DOS; BIOS Р·Р°РґР°СЃС‚ Р»РѕРєР°Р»СЊРЅС‹Р№ РІС…РѕРґ.
    ld hl,localBlockImage
    ld de,localBlockImage+BANK_MAIN_LOCAL_BYTES
    ld bc,BOOT_LOCAL_BLOCK
    call moveCore
    ld hl,clientImage
    ; CLIENT РѕРґРёРЅР°РєРѕРІ РІ MAIN Рё DOS; СЃР»РѕС‚ AT512KS СЂР°СЃРїРѕР»РѕР¶РµРЅ РІ LOCAL.
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
    xor a
    ld (0FFFCh),a
    jp 0C000h
bootFailed:
    halt
    jp bootFailed

; A=С„РёР·РёС‡РµСЃРєР°СЏ СЃС‚СЂР°РЅРёС†Р°, DE=РёСЃС‚РѕС‡РЅРёРє main, HL=РЅР°Р·РЅР°С‡РµРЅРёРµ, BC=РґР»РёРЅР°.
; РћР±С‰РёР№ Р±Р°Р№С‚РѕРІС‹Р№ СѓСЃС‚Р°РЅРѕРІС‰РёРє РЅСѓР¶РµРЅ С‚РѕР»СЊРєРѕ РґР»СЏ С…РІРѕСЃС‚РѕРІ РґРѕ Р·Р°РїСѓСЃРєР° С‚СЂР°РЅСЃРїРѕСЂС‚Р°.
install:
    ld (0FFC1h),a
installLoop:
    call readSource
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
; DE: РІСЂРµРјРµРЅРЅС‹Рµ РґР°РЅРЅС‹Рµ stage РЅР°С…РѕРґСЏС‚СЃСЏ РІ MAIN, MEM.SYS вЂ” РІ ROM 7000h.
readSource:
    IFNDEF ORIGINAL_MX
    ld a,d
    or a
    jp m,readSourceRam
    xor a
    ld (0FFFEh),a
    ld a,(de)
    push af
    xor a
    ld (0FFFCh),a
    pop af
    ret
readSourceRam:
    ENDIF
    ld a,(de)
    ret
byteWindow:
    ld a,0
    ld (0FFFDh),a
    ld (hl),c
    ld (0FFFCh),a
    ret
byteRead:
    ; Р’С‚РѕСЂРѕР№ РІС…РѕРґ С‡РёС‚Р°РµС‚ Р±Р°Р№С‚ РІ C; РЅРѕРјРµСЂ СЃС‚СЂР°РЅРёС†С‹ Р·Р°РґР°С‘С‚СЃСЏ РїРµСЂРµРґ РїСЂРѕРІРµСЂРєРѕР№.
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
    IFNDEF ORIGINAL_MX
    INCLUDE "splash.inc"
    ENDIF
    INCLUDE "memcpy.inc"
bootFontName:   DB BOOT_SELECTED_BASENAME
pageCount:      DB 16
ramFresh:       DB 1
dosKind:        DB 2
    IFDEF ORIGINAL_MX
dosLocalA:      DW MX_DOS_LOCAL_A
dosNearCopy:    DW MX_DOS_NEAR_COPY
    ELSE
dosLocalA:      DW BOOT_DOS_LOCAL_A
dosNearCopy:    DW BOOT_DOS_NEAR_COPY
    ENDIF
dosContext:
    DB 1
    ; Р§РёСЃР»Рѕ СЃС‚СЂР°РЅРёС† РїРѕСЃР»Рµ РґРµС‚РµРєС‚РѕСЂР° РєРѕРїРёСЂСѓРµС‚СЃСЏ СЃСЋРґР° РїРµСЂРµРґ СѓСЃС‚Р°РЅРѕРІРєРѕР№.
    DB 16,0,0,0
    DW BOOT_DOS_STACK_TOP,BOOT_DOS_STACK_BOTTOM+64
dosContextEnd:
memImage        = BOOT_MEM_ROM
transportImage  = memImage+MEM_TRANSPORT_OFFSET
transportEnd    = transportImage+MEM_TRANSPORT_SIZE
clientImage     = memImage+MEM_CLIENT_OFFSET
clientEnd       = clientImage+MEM_CLIENT_SIZE
localBlockImage = memImage+MEM_LOCAL_OFFSET
localBlockEnd   = localBlockImage+MEM_LOCAL_SIZE
windowImage     = memImage+MEM_WINDOW_OFFSET
windowEnd       = windowImage+MEM_WINDOW_SIZE
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
romTailImage = BOOT_STAGE_BASE+romTailStorage-stageImage
romTailEnd   = romTailImage+romTailStorageEnd-romTailStorage
stageEnd:
    ASSERT stageEnd <= 03000h
    ; РҐРѕР»РѕРґРЅС‹Р№ bootstrap РІСЂРµРјРµРЅРЅРѕ Р·Р°РЅРёРјР°РµС‚ С‚Р°РєР¶Рµ СЃРІРѕР±РѕРґРЅСѓСЋ РѕР±Р»Р°СЃС‚СЊ RUN.
    ; РЎС‚РµРє Рё РїРµСЂРµРјРµРЅРЅС‹Рµ BIOS РѕСЃС‚Р°СЋС‚СЃСЏ РІС‹С€Рµ BOOT_STAGE_LIMIT.
    ASSERT BOOT_STAGE_BASE+stageEnd-stageImage <= BOOT_STAGE_LIMIT
