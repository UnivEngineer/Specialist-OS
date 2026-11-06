; Копировщики системного ROM: через main-буфер и напрямую в ДОЗУ.
; Вызывающая программа сохраняет и восстанавливает окно и хвосты страниц.
; Чтение ROM страницы 0, 2000h..3FFFh, без изменения flash.
readROM:
    ld hl,readerImage
    ld de,0FFC0h
    ld bc,readerEnd-readerImage
    call localCopy
    ld hl,EXPECTED
    ld (expectedPointer),hl
    ld hl,02000h
    ld c,32
readROMLoop:
    ld de,08800h
    ld b,0
    call 0FFC0h
    ; При ROM нижние 32 КиБ недоступны для записи в main RAM.
    ; Только после отключения ROM переносим верхний буфер в EXPECTED.
    push hl
    push bc
    LOAD_DE expectedPointer
    ld hl,08800h
    ld bc,256
    call localCopy
    ex de,hl
    ld (expectedPointer),hl
    pop bc
    pop hl
    dec c
    jp nz,readROMLoop
    ret
readerImage:
    DISP 0FFC0h
    xor a
    ld (IO_PAGE_ROM),a
readerLoop:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,readerLoop
    ld (IO_PAGE_RAM),a
    ret
    ENT
readerEnd:
    ASSERT readerEnd-readerImage <= 32
; ROM->ДОЗУ: 32 сектора, источник 2000h, назначение TARGET.
; SP используется для данных, поэтому между переключениями нет CALL/RET.
directCopy:
    ld hl,romWindow
    ld de,0FFC0h
    ld bc,19
    call localCopy
    ld hl,0
    add hl,sp
    ld (0FFD4h),hl
    ld a,(targetPage)
    ld (0FFCCh),a
    ld a,32
    ld (0FFD3h),a
    ld hl,TARGET+4
    jp 0FFC0h
romWindow:
    ld sp,02000h
    ld a,0
    ld (IO_PAGE_ROM),a
    jp romRead4
    ld a,0
    ld (IO_PAGE_RAMD),a
    jp romWrite4
romTailImage:
    DISP 0FF00h
romRead4:
    pop bc
    pop de
    jp 0FFCBh
romWrite4:
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
    ld (IO_PAGE_RAM),a
    ld hl,(0FFD4h)
    ld sp,hl
    ret
    ENT
romTailImageEnd:
    ASSERT romTailImageEnd-romTailImage <= 192
