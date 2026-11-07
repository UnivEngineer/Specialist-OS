; Второй этап: исполняется из верхней main RAM, BIOS ещё не используется.
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
    ; Внешний файл шрифта лежит в первой ROM-странице отдельно от BIOS.
    ld hl,BOOT_FONT_ROM
    ld de,BOOT_FONT_ROM+BOOT_FONT_BYTES
    ld bc,BOOT_FONT_RAM
    call moveCore
fontLoaded:
    ; Обе части системы готовятся до холодного входа C000h.
    ; Код чтения уже находится в BIOS, нижний DOS можно писать по месту.
    call BIOS_BOOT_LOAD_DOS
    jp c,bootFailed
dosLoaded:
    IF BOOT_PROBE
    ; Установить окно записи одиночного байта в физическую страницу 0.
    ld hl,byteWindow
    ld de,byteWindow+byteWindowEnd-byteWindow
    ld bc,0FFC0h
    call moveCore
    ld hl,0FF00h
    ld de,tailImage
    ld b,tailEnd-tailImage
installTail:
    ld a,(de)
    ld c,a
    call 0FFC0h
    inc hl
    inc de
    dec b
    jp nz,installTail
    ld hl,tailImage
    ld de,tailEnd
    ld bc,0FF00h
    call moveCore
    ld hl,window
    ld de,windowEnd
    ld bc,0FFC0h
    call moveCore
    ld hl,0
    add hl,sp
    ; Следующий CALL положит адрес возврата на два байта ниже.
    dec hl
    dec hl
    ld (0FFD4h),hl
    ld a,MODULE_SECTORS
    ld (0FFD3h),a
copyModuleStart:
    ld hl,4
    call 0FFC0h
    ENDIF
bootDone:
    ; Возврат к первой ROM-странице; C000h доступен и при включённом ROM.
    xor a
    ld (0FFFEh),a
    jp 0C000h
bootFailed:
    ; BIOS сможет показать диагностику после установки шрифта и экрана.
    jp 0C000h
    INCLUDE "memcpy.inc"
byteWindow:
    xor a
    ld (0FFFDh),a
    ld (hl),c
    ld (0FFFCh),a
    ret
byteWindowEnd:
window:
    ld sp,0
    ld a,1
    ld (0FFFEh),a
    jp romRead4
    ld a,0
    ld (0FFFDh),a
    jp ramWrite4
windowEnd:
    ASSERT windowEnd-window == 19
; Одинаковый код по FF00h в main и RAMD0; SP переносит данные, не вызовы.
    ENT
tailStorage:
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
tailStorageEnd:
tailImage = 08000h+tailStorage-stageImage
tailEnd = tailImage+tailStorageEnd-tailStorage
    ASSERT tailEnd-tailImage <= 192
stageEnd:
    ASSERT stageEnd <= 03000h
