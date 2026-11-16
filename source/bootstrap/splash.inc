; Historical Specialist MX2 bitmap from RAMFOS-master/logo.inc (vinxru, 2013).
; White letters: independent column RLE streams. Red superscript 2: raw bitmap.
; Empty top/bottom rows are omitted; the boot sector already cleared the screen.
    INCLUDE "../../bin/bootstrap/splash-config.inc"
drawBootLogo:
    ld a,0F0h
    ld (BOOT_COLOR_PORT),a
    ld de,bootLogoText
    ld hl,09100h+BOOT_LOGO_TEXT_Y
    call drawBootLogoText
    ld a,0C0h
    ld (BOOT_COLOR_PORT),a
    ld de,bootLogoDigit
    ld h,0BDh
    ld c,2
.column:
    ld b,BOOT_LOGO_DIGIT_HEIGHT
    ld l,BOOT_LOGO_DIGIT_Y
.byte:
    ld a,(de)
    inc de
    ld (hl),a
    inc hl
    dec b
    jp nz,.byte
    inc h
    dec c
    jp nz,.column
    ld a,0F0h
    ld (BOOT_COLOR_PORT),a
    ret

; Packets never cross a column: B=remaining rows, C=packet length.
; Control 0..127: copy 1..128 literals; 128..255: repeat next byte 1..128 times.
drawBootLogoText:
.column:
    ld b,BOOT_LOGO_TEXT_HEIGHT
.packet:
    ld a,(de)
    inc de
    or a
    jp m,.repeat
    inc a
    ld c,a
    ld a,b
    sub c
    ld b,a
.literal:
    ld a,(de)
    inc de
    ld (hl),a
    inc hl
    dec c
    jp nz,.literal
    jp .packetDone
.repeat:
    and 07Fh
    inc a
    ld c,a
    ld a,b
    sub c
    ld b,a
    ld a,(de)
    inc de
.run:
    ld (hl),a
    inc hl
    dec c
    jp nz,.run
.packetDone:
    ld a,b
    or a
    jp nz,.packet
    inc h
    ld l,BOOT_LOGO_TEXT_Y
    ld a,h
    cp 0BDh
    jp nz,.column
    ret
bootLogoText:
    INCBIN "../../bin/bootstrap/splash-text.rle"
bootLogoDigit:
    INCBIN "../../bin/bootstrap/splash-digit.bin"
bootLogoEnd:
    ASSERT bootLogoEnd-bootLogoDigit == 2*BOOT_LOGO_DIGIT_HEIGHT
