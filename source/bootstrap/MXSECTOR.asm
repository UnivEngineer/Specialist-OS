; Линейное ПЗУ MX. После переноса загрузчик работает в верхнем ОЗУ,
; доступном в MAIN и при подключённом ПЗУ.
    INCLUDE "../common/system.inc"
    IFDEF MX_BOOT_MINIMAL
    ORG 0000h
    ELSE
    ORG MX_ROM_BOOT_ORIGIN
    jp coldStart
    jp recoverFloppy
    jp recoverPrompt
    ENDIF
coldStart:
    di
    ; Сначала переносим и выполняем только очистку экрана.
    ld hl,clearImage
    ld de,0C000h
    ld bc,clearEnd-clearImage
    ld sp,0EFF0h
    call copy
    jp 0C000h
cleared:
    ld hl,0C000h
    jp relocateLoader
recoverFloppy:
    ld hl,floppyBoot
    jp relocateLoader
recoverPrompt:
    ld hl,noSystem
relocateLoader:
    ld sp,0EFF0h
    push hl
    ld hl,loader
    ld de,0C000h
    ld bc,loaderEnd-loader
    jp copy
copy:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,copy
    ret
clearImage:
    DISP 0C000h
    ld (IO_PAGE_RAM),a
    ld a,0F0h
    ld (IO_COLOR),a
    ld sp,0C000h
    ld hl,0
    ld bc,03000h/8
.clear:
    DUP 4
        push hl
    EDUP
    dec bc
    ld a,b
    or c
    jp nz,.clear
    ld sp,0EFF0h
    ld (IO_PAGE_ROM),a
    jp cleared
    ENT
clearEnd:
    IFDEF MX_BOOT_MINIMAL
    ASSERT $ <= 0103h
    DS 0103h-$,0FFh
    ; Общие со старшими образами точки возврата второй ступени.
    jp recoverFloppy
    jp recoverPrompt
    ENDIF
loader:
    DISP 0C000h
    ld (IO_PAGE_RAM),a
    ld sp,0EFF0h
    IFNDEF MX_BOOT_MINIMAL
    call drawBootLogo
    ENDIF

; LBA 1: сигнатура MXB1 или MXF1, число 512-байтных секторов от LBA 2 и сумма.
; Ступень загружается с 3000h и запускается с 8000h.
    ld a,1
    call tryMedia
floppyBoot:
    ld (IO_PAGE_RAM),a
    xor a
    call tryMedia
noSystem:
    ld (IO_PAGE_RAM),a
    ld a,0F0h
    ld (BOOT_COLOR_PORT),a
    ld hl,bootMessage
    ld de,BOOT_MESSAGE_SCREEN
    ld b,BOOT_MESSAGE_COLS
.column:
    push bc
    ld b,8
.row:
    ld a,(hl)
    ld (de),a
    inc hl
    inc e
    dec b
    jp nz,.row
    ld e,BOOT_MESSAGE_Y
    inc d
    pop bc
    dec b
    jp nz,.column
    halt

tryMedia:
    ld (bootMedium),a
    ld de,1
    ld hl,0
    call readSector
    ret c
    ld hl,(0)
    ld de,0584Dh
    call equal
    ret nz
    ld hl,(2)
    ld de,03142h
    ld a,(bootMedium)
    or a
    jp z,.signature
    ld e,046h
.signature:
    call equal
    ret nz
    ld a,(4)
    cp 41                     ; entry at 8000h must be covered
    ret c
    cp 48                     ; exclusive limit 8E00h
    ret nc
    ld c,a
    ld de,2
    ld hl,03000h
.next:
    push bc
    call readSector
    pop bc
    ret c
    inc h
    inc h
    inc de
    dec c
    jp nz,.next
    ex de,hl
    ld hl,03000h
    ld bc,0
.sum:
    ld a,(hl)
    add a,c
    ld c,a
    jp nc,.advance
    inc b
.advance:
    inc hl
    call equal
    jp nz,.sum
    ld hl,(5)
    ld d,b
    ld e,c
    call equal
    ret nz
    ld a,(bootMedium)
    rrca                    ; CF=1 для flash, CF=0 для дискеты
    jp 08000h
equal:
    ld a,h
    cp d
    ret nz
    ld a,l
    cp e
    ret
readSector:
    ld a,(bootMedium)
    or a
    jp nz,mxFlashRead
    push hl
    push de
    ld b,2
    call fdTransfer
    pop de
    pop hl
    ret
bootMedium: DB 0
bootMessage:
    INCBIN "../../bin/mx/prompt.bin"
bootMessageEnd:
BOOT_MESSAGE_COLS   = (bootMessageEnd-bootMessage)/8
BOOT_MESSAGE_X      = (48-BOOT_MESSAGE_COLS)/2
BOOT_MESSAGE_Y      = 8
BOOT_MESSAGE_SCREEN = 09000h+BOOT_MESSAGE_X*256+BOOT_MESSAGE_Y
    INCLUDE "mx-flash-read.inc"
BOOT_COLOR_PORT = IO_COLOR
    INCLUDE "../floppy/transfer.inc"
    IFNDEF MX_BOOT_MINIMAL
    INCLUDE "splash.inc"
    ENDIF
    ENT
loaderEnd:
    IFDEF MX_BOOT_MINIMAL
    ASSERT loaderEnd <= 0800h
    DS 0800h-$,0FFh
    ELSE
    ASSERT loaderEnd <= 08000h
    ENDIF
