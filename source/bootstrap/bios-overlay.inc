;----------------------------------------------------------------------------
; MXOS
; Разложить накладки BIOS в MAIN и сохранить эталон для возврата из MON/RF.
; вход: MAIN и банк DOS доступны, общий шаблон MEM уже установлен.
; выход: накладки установлены, эталон лежит перед кодом DOS. AF, BC, DE и HL
; изменены, SP восстановлен, выбран MAIN. Общее окно затем ставит BOOTBANK.
;----------------------------------------------------------------------------

installMainBiosOverlay:
    ld de,bootMainOverlay
    ld hl,BOOT_BIOS_OVERLAY_DOS
    ld bc,bootMainOverlayEnd-bootMainOverlay
    xor a
    call install
    ld hl,bootMainOverlay
.record:
    ld a,(hl)
    inc hl
    or a
    ret z
    ld b,a
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
.copy:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,.copy
    jp .record

bootMainOverlay:
    INCBIN "../../bin/BIOS.main-overlay.bin"
bootMainOverlayEnd:
