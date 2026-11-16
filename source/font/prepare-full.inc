; Disposable 10 KiB generator: one complete 256-byte row page per loop.
; Source and destination are page-aligned. Last two phases reuse the first.
prepareFull:
    ld hl,(lowerBase)
    ex de,hl
    call prepareSource
    ld b,8
buildPage0:
buildLoop0:
    ld a,(hl)
    add a,a
    add a,a
    ld (de),a
    inc l
    inc e
    jp nz,buildLoop0
    inc h
    inc d
    dec b
    jp nz,buildPage0
    call prepareSource
    ld b,8
buildPage1:
buildLoop1:
    ld a,(hl)
    rrca
    rrca
    and 0Fh
    ld (de),a
    inc l
    inc e
    jp nz,buildLoop1
    inc h
    inc d
    dec b
    jp nz,buildPage1
    call prepareSource
    ld b,8
buildPage2:
buildLoop2:
    ld a,(hl)
    rrca
    rrca
    and 0C0h
    ld (de),a
    inc l
    inc e
    jp nz,buildLoop2
    inc h
    inc d
    dec b
    jp nz,buildPage2
    ; b>>4 из готовой плоскости b>>2: ещё два вращения.
    ld hl,(lowerBase)
    ld a,h
    add a,8
    ld h,a
    ld b,8
buildPage3:
buildLoop3:
    ld a,(hl)
    rrca
    rrca
    and 3
    ld (de),a
    inc l
    inc e
    jp nz,buildLoop3
    inc h
    inc d
    dec b
    jp nz,buildPage3
    ; b<<4 из готовой плоскости b<<2: ещё два сложения.
    ld hl,(lowerBase)
    ld b,8
buildPage4:
buildLoop4:
    ld a,(hl)
    add a,a
    add a,a
    ld (de),a
    inc l
    inc e
    jp nz,buildLoop4
    inc h
    inc d
    dec b
    jp nz,buildPage4
    ret
