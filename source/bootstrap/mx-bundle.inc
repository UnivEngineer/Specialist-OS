;----------------------------------------------------------------------------
; MXOS
;
; Выбрать системный комплект. Нулевой байт в слоте BIOS означает запуск с C:.
; Вход: проверенная ступень в MAIN. Стек и код находятся в MAIN.
; Выход: CF=0 при успехе, CF=1 при несовместимом C:. AF/BC/DE/HL изменены.
;----------------------------------------------------------------------------
mxPickBundle:
    ld a,(03000h)
    or a
    ret nz
    ld hl,mxRomCopy
    ld de,mxRomCopyEnd
    ld bc,0FFC0h
    call moveCore
    ld hl,0120h
    ld de,mxRomDirectory
    ld bc,mxRomDirectoryEnd-mxRomDirectory
    call 0FFC0h
    ld hl,(mxRomDirectory)
    ld de,MX_ROM_BUNDLE_ID & 0FFFFh
    call mxBundleWord
    jp nz,mxBundleMissing
    ld hl,(mxRomDirectory+2)
    ld de,MX_ROM_BUNDLE_ID >> 16
    call mxBundleWord
    jp nz,mxBundleMissing
    ld hl,(mxRomDirectory+4)
    ld de,03000h
    ld bc,DOS_BYTES
    call 0FFC0h
    ld hl,(mxRomDirectory+6)
    ld de,BOOT_FONT_ROM
    ld bc,MX_ROM_FONT_BYTES
    call 0FFC0h
    ld hl,(mxRomDirectory+8)
    ld de,BOOT_IO_ROM
    ld bc,MX_ROM_IO_BYTES
    call 0FFC0h
    ld hl,(mxRomDirectory+10)
    ld de,BOOT_MEM_ROM
    ld bc,MX_ROM_MEM_BYTES
    call 0FFC0h
    ld a,1
    ld (mxRomBundle),a
    or a
    ret
mxBundleMissing:
    scf
    ret
mxBundleWord:
    ld a,h
    cp d
    ret nz
    ld a,l
    cp e
    ret

; HL=ROM, DE=MAIN, BC>0. Стек трогается только после возврата в MAIN.
mxRomCopy:
    ld (0FFFEh),a
    ld a,(hl)
    ld (0FFFCh),a
    ld (de),a
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,0FFC0h
    ret
mxRomCopyEnd:
    ASSERT mxRomCopyEnd-mxRomCopy <= 32
mxRomDirectory:
    DS 12,0
mxRomDirectoryEnd:

;----------------------------------------------------------------------------
; MXOS
;
; Проверить готовность ядра и соответствие системного комплекта и носителя.
; Вход: byteWindow в FFC0h, выбран физический банк DOS 0.
; Выход: Z=1 и BIOS_BOOT_WARM=1 при совпадении. AF/BC/DE/HL изменены.
;----------------------------------------------------------------------------
mxCheckWarm:
    ld hl,DOS_READY
    call 0FFC0h+byteRead-byteWindow
    ld a,c
    cp 1
    ret nz
    ld hl,MX_BOOT_COOKIE
    ld de,mxBootMedium
    ld b,mxBootIdentityEnd-mxBootMedium
.compare:
    call 0FFC0h+byteRead-byteWindow
    ld a,(de)
    cp c
    ret nz
    inc hl
    inc de
    dec b
    jp nz,.compare
    ld a,1
    ld (BIOS_BOOT_WARM),a
    xor a
    ret
