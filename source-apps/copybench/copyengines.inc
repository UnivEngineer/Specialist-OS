; Экспериментальные входы из main, не опубликованный ABI BIOS.
; HL -> стандартный FAR-пакет. Логические страницы 0=main, 1..16=ДОЗУ.
; Block256: ровно 256 байт, источник выровнен по 256, диапазоны проверяет
; вызывающий драйвер. AF/BC/DE/HL разрушены, SP сохранён; окно рабочее.
; Universal: произвольная длина/выравнивание, ноль допустим; диапазоны до
; FF00h, без пересечений, IRQ запрещены. BC/DE/HL, SP и окно сохраняются.
; Выход universal: A=статус, CF=ошибка. Хвосты устанавливаются заранее.
r4Block256:
    ld de,bankCopyRequest
    ld b,8
    call bankCopyBytes
    ld hl,r4BlockWindow
    ld de,0FFC0h
    ld b,19
    call bankCopyBytes
    ld hl,(bankCopyRequest+1)
    ld (0FFC1h),hl
    ld a,(bankCopyRequest)
    call bankCopySelector
    ld (0FFC6h),a
    ld a,c
    ld (0FFC4h),a
    ld a,(bankCopyRequest+3)
    call bankCopySelector
    ld (0FFCEh),a
    ld a,c
    ld (0FFCCh),a
    ld hl,0
    add hl,sp
    ld (0FFD3h),hl
    ld hl,(bankCopyRequest+4)
    inc hl
    inc hl
    inc hl
    inc hl
    jp 0FFC0h
r4BlockWindow:
    ld sp,0
    ld a,0
    ld (0FFFDh),a
    jp r4BlockRead
    ld a,0
    ld (0FFFDh),a
    jp r4BlockWrite
r4BlockTailImage:
    DISP 0FF00h
r4BlockRead:
    pop bc
    pop de
    jp 0FFCBh
r4BlockWrite:
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
    ; Конец сектора; main содержит тот же хвост для безопасного возврата.
    ld (0FFFCh),a
    ld hl,(0FFD3h)
    ld sp,hl
    ret
    ENT
r4BlockTailEnd:

cpmBlock256:
    ld de,bankCopyRequest
    ld b,8
    call bankCopyBytes
    call cpmSetup
    ld hl,16
    ld (0FFD4h),hl
    jp 0FF00h

cpmUniversal:
    push bc
    push de
    push hl
    ld de,bankCopyRequest
    ld b,8
    call bankCopyBytes
    ld hl,(bankCopyRequest+6)
    ld a,h
    or l
    jp z,bankCopyOK
    ld a,(bankCopyRequest)
    call bankCopyPageCheck
    jp c,bankCopyError
    ld a,(bankCopyRequest+3)
    call bankCopyPageCheck
    jp c,bankCopyError
    ld hl,(bankCopyRequest+1)
    call bankCopyRange
    jp c,bankCopyError
    ld hl,(bankCopyRequest+4)
    call bankCopyRange
    jp c,bankCopyError
    ld hl,0FFC0h
    ld de,bankCopySavedWindow
    ld b,32
    call bankCopyBytes
    call cpmSetup
    ld hl,(bankCopyRequest+6)
    ld a,l
    and 15
    ld (cpmRemainder),a
    ld b,4
cpmDivide16:
    or a
    ld a,h
    rra
    ld h,a
    ld a,l
    rra
    ld l,a
    dec b
    jp nz,cpmDivide16
    ld (0FFD4h),hl
    ld a,h
    or l
    call nz,0FF00h
    ld a,(cpmRemainder)
    or a
    jp z,cpmRestore
    ; Оригинальный CP/M обновил адреса на размер полных порций.
    ld hl,(0FFD0h)
    ld (cpmSource),hl
    ld hl,(0FFD2h)
    ld (cpmDestination),hl
    call byteMode
    ld hl,(cpmSource)
    LOAD_DE cpmDestination
    ld a,(cpmRemainder)
    ld b,a
cpmByteLoop:
    ld a,(bankCopyRequest)
    or a
    jp z,cpmReadMain
    dec a
    call 0FFC0h
    jp cpmWriteByte
cpmReadMain:
    ld c,(hl)
cpmWriteByte:
    ex de,hl
    ld a,(bankCopyRequest+3)
    or a
    jp z,cpmWriteMain
    dec a
    call 0FFD0h
    jp cpmByteNext
cpmWriteMain:
    ld (hl),c
cpmByteNext:
    inc hl
    ex de,hl
    inc hl
    dec b
    jp nz,cpmByteLoop
cpmRestore:
    ld hl,bankCopySavedWindow
    ld de,0FFC0h
    ld b,32
    call bankCopyBytes
    jp bankCopyOK
; Параметры исходного CP/M из локальной копии запроса, возврат в main.
; Все регистры разрушены. FFh обозначает main в исходном CP/M.
cpmSetup:
    ld hl,(bankCopyRequest+1)
    ld (0FFD0h),hl
    ld hl,(bankCopyRequest+4)
    ld (0FFD2h),hl
    ld a,(bankCopyRequest)
    dec a
    ld (0FFD6h),a
    ld a,(bankCopyRequest+3)
    dec a
    ld (0FFD7h),a
    ld a,0FFh
    ld (0FFD8h),a
    ret
cpmSource: DW 0
cpmDestination: DW 0
cpmRemainder: DB 0
