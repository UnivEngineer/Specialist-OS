; До первой записи расширенного хвоста: старую/неизвестную разметку не стираем.
; Поддерживается текущий MXOS256: 256 B, SPC=1, reserved=1, FAT=1, root=64.
; Читаем FAT только из первого банка B:; никакой записи в том нет.
guardRamDisk:
    ld a,1
    ld (0FFC0h+byteRead-byteWindow+1),a
    ld hl,3
    call guardReadWord
    ld a,e
    cp 'M'
    jp nz,guardUnknown
    ld a,d
    cp 'X'
    jp z,guardKnown
guardUnknown:
    ld hl,11
    call guardReadWord
    ld a,e
    or a
    jp nz,guardCheck128
    ld a,d
    cp 1
    jp z,guardBlocked
    cp 2
    jp z,guardBlocked
    ret
guardCheck128:
    ld a,d
    or a
    ret nz
    ld a,e
    cp 128
    jp z,guardBlocked
    ret
guardKnown:
    ld hl,11
    call guardReadWord
    ld a,e
    or a
    jp nz,guardBlocked
    ld a,d
    cp 1
    jp nz,guardBlocked
    ld hl,13
    call guardReadWord
    ld a,e
    cp 1
    jp nz,guardBlocked
    ld a,d
    cp 1
    jp nz,guardBlocked
    ld hl,15
    call guardReadWord
    ld a,e
    or a
    jp nz,guardBlocked
    ld a,d
    cp 1
    jp nz,guardBlocked
    ld hl,17
    call guardReadWord
    ld a,e
    cp 64
    jp nz,guardBlocked
    ld a,d
    or a
    jp nz,guardBlocked
    ld hl,19
    call guardReadWord
    ld a,e
    or a
    jp nz,guardBlocked
    ld a,d
    or a
    jp z,guardBlocked
    ld (guardPages),a
    ld a,(pageCount)
    cp d
    jp c,guardBlocked
    jp z,guardBlocked
    ld hl,22
    call guardReadWord
    ld a,d
    or a
    jp nz,guardBlocked
    ld a,(guardPages)
    add a,a
    cp e
    jp nz,guardBlocked
    ld a,e
    add a,9
    ld l,a
    ld h,0
    ld (guardDataStart),hl
    ld a,1
    ld (guardIndex),a
guardNextBank:
    ld hl,(guardDataStart)
    ex de,hl
    ld a,(guardIndex)
    ld h,a
    ld l,0
    ld a,l
    sub e
    ld l,a
    ld a,h
    sbc a,d
    ld h,a
    add hl,hl                 ; Смещение FAT16-элемента для сектора FEh.
    inc h                     ; FAT начинается с LBA 1 в первом банке B:.
    call guardCheckBad
    inc hl
    call guardCheckBad        ; Следующий элемент защищает сектор FFh.
    ld a,(guardIndex)
    ld hl,guardPages
    cp (hl)
    jp z,guardAccepted
    inc a
    ld (guardIndex),a
    jp guardNextBank
guardAccepted:
    xor a
    ld (ramFresh),a            ; Готовый том сохраняется даже при новом MEM.
    ret
guardCheckBad:
    call guardReadWord
    ld a,e
    cp 0F7h
    jp nz,guardBlocked
    ld a,d
    cp 0FFh
    jp nz,guardBlocked
    ret
guardReadWord:
    call 0FFC0h+byteRead-byteWindow
    ld e,c
    inc hl
    call 0FFC0h+byteRead-byteWindow
    ld d,c
    ret
guardBlocked:
    ; Пишем только MAIN; stage 8000h..8DFFh остаётся целым.
    ld a,0F0h
    ld (BOOT_COLOR_PORT),a
    ld bc,guardMessage
    ld hl,09008h
guardMessageNext:
    ld a,(bc)
    inc bc
    or a
    jp z,guardHalt
    ld e,a
    ld d,BOOT_FONT_RAM >> 8
    push hl
guardGlyphRow:
    ld a,(de)
    ld (hl),a
    inc l
    inc d
    ld a,d
    cp (BOOT_FONT_RAM >> 8)+8
    jp nz,guardGlyphRow
    pop hl
    inc h
    jp guardMessageNext
guardHalt:
    halt
    jp guardHalt
guardMessage: DB 'OLD RAM DISK - BACK UP FIRST',0
guardPages: DB 0
guardIndex: DB 0
guardDataStart: DW 0
