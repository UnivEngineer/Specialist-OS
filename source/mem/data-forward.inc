; Прямой MOVE в DATA. Устанавливается поверх подготовки прямого хода
; полного профиля, начиная с compact223Forward, только в профиле DATA.
; HL — источник, DE — назначение, BC — длина в байтах. SP восстановлен.

moveDataOverlay:
    ld a,c
    rrca
    jp nc,moveDataOverlayEven
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
moveDataOverlayEven:
    push hl
    ld hl,2
    add hl,sp
    ld (moveDataOverlayReturn+1),hl
    pop hl
    ld sp,hl
    ex de,hl
    ; B — число полных групп по 128 пар плюс начальный остаток.
    ld a,c
    rrca
    and 7Fh
    ld c,a
    inc b
    or a
    jp z,moveDataOverlayNext
    rrca
    jp c,moveDataOverlayOdd
moveDataOverlayPair:
    pop de
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    dec c
moveDataOverlayOdd:
    pop de
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    dec c
    jp nz,moveDataOverlayPair
moveDataOverlayNext:
    dec b
    jp z,moveDataOverlayReturn
    ld c,128
    jp moveDataOverlayPair
moveDataOverlayReturn:
    ld sp,0
    ret
moveDataOverlayEnd:
