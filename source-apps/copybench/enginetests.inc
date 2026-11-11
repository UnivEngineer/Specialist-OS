; Сопоставимая матрица: четыре реализации, main <-> RAMD и RAMD -> RAMD.
; Все переносы 8192 байта. Блочные входы вызываются 32 раза, универсальные — один.
engineTests:
    ld a,(bios_upper.ramPageCount)
    ld (bankPageCount),a
    xor a
    ld (engineBank),a
    ld (engineMode),a
    ld hl,engineTimings
    ld (engineOutput),hl
enginePair:
    call engineName
    call puts
    ld hl,pairText
    call puts
    ld hl,EXPECTED
    ld (farRequest+1),hl
    ld hl,TARGET
    ld (farRequest+4),hl
    xor a
    ld (farRequest),a
    ld a,(targetPage)
    inc a
    ld (farRequest+3),a
    call engineMeasure
    call enginePrintTime
    ld a,(engineStatus)
    or a
    ld hl,pairSeparator
    jp z,enginePrintSlash
    ld hl,pairFailedSeparator
enginePrintSlash:
    call puts
    ; Меняем направление: в банке остаётся эталон, main инвертируется перед тестом.
    ld hl,TARGET
    ld (farRequest+1),hl
    ld hl,EXPECTED
    ld (farRequest+4),hl
    ld a,(targetPage)
    inc a
    ld (farRequest),a
    xor a
    ld (farRequest+3),a
    call engineMeasure
    call enginePrintTime
    ld a,(engineStatus)
    call result
    call readROM
    ld hl,engineMode
    inc (hl)
    ld a,(hl)
    cp 4
    jp c,enginePair
    ret

bankTests:
    ld a,(bios_upper.ramPageCount)
    cp 2
    jp c,engineSkipBank
    ld a,(targetPage)
    ld (sourcePhysical),a
    dec a
    ld (destinationPhysical),a
    ld (targetPage),a
    call byteMode
    ld hl,TARGET
    ld de,EXPECTED
    ld bc,LENGTH
    call bankRead
    ld hl,0FF00h
    ld de,otherTail
    ld bc,192
    call bankRead
    ld a,(sourcePhysical)
    ld (targetPage),a
    ld a,1
    ld (engineBank),a
    xor a
    ld (engineMode),a
engineBankLoop:
    ld a,(sourcePhysical)
    inc a
    ld (farRequest),a
    ld a,(destinationPhysical)
    inc a
    ld (farRequest+3),a
    ld hl,TARGET
    ld (farRequest+1),hl
    ld (farRequest+4),hl
    call engineMeasure
    call engineName
    call puts
    ld hl,engineBankText
    call puts
    call enginePrintTime
    ld a,(engineStatus)
    call result
    ld hl,engineMode
    inc (hl)
    ld a,(hl)
    cp 4
    jp c,engineBankLoop
    call byteMode
    ld a,(destinationPhysical)
    ld (targetPage),a
    ld hl,TARGET
    ld de,EXPECTED
    ld bc,LENGTH
    call bankWrite
    ld hl,0FF00h
    ld de,otherTail
    ld bc,192
    call bankWrite
    ld a,(sourcePhysical)
    ld (targetPage),a
    ret
engineSkipBank:
    ld hl,farSkipText
    jp puts

engineMeasure:
    ld hl,LENGTH
    ld (farRequest+6),hl
    call matrixPoison
    call engineInstall
    call reset_timing
    call engineRun
    ld (engineStatus),a
    call timer_sample
    call matrixCompare
    or a
    jp z,engineChecked
    ld (engineStatus),a
engineChecked:
    ld a,(engineStatus)
    or a
    jp z,engineSaveTicks
    ld hl,engineErrors
    inc (hl)
engineSaveTicks:
    ld hl,v_elapsed
    LOAD_DE engineOutput
    ld bc,4
    call localCopy
    ex de,hl
    ld (engineOutput),hl
    ret
enginePrintTime:
    ld hl,v_elapsed
    jp print_time_ms
engineName:
    ld a,(engineMode)
    add a,a
    ld e,a
    ld d,0
    ld hl,engineNames
    add hl,de
    call getWord
    ex de,hl
    ret

; Установка хвоста во всех участвующих страницах — до запуска таймера.
engineInstall:
    ld a,(engineMode)
    and 1
    ld hl,cpmBlob
    ld bc,164
    jp nz,engineTailReady
    ld a,(engineMode)
    or a
    ld hl,r4BlockTailImage
    ld bc,r4BlockTailEnd-r4BlockTailImage
    jp z,engineTailReady
    ld hl,farTailImage
    ld bc,farTailEnd-farTailImage
engineTailReady:
    ld (engineTail),hl
    ld h,b
    ld l,c
    ld (engineTailSize),hl
    call byteMode
    ld hl,0FF00h
    LOAD_DE engineTail
    push hl
    ld hl,(engineTailSize)
    ld b,h
    ld c,l
    pop hl
    call bankWrite
    ld a,(engineBank)
    or a
    jp z,engineMainTail
    ld a,(destinationPhysical)
    ld (targetPage),a
    ld hl,0FF00h
    LOAD_DE engineTail
    push hl
    ld hl,(engineTailSize)
    ld b,h
    ld c,l
    pop hl
    call bankWrite
    ld a,(sourcePhysical)
    ld (targetPage),a
engineMainTail:
    ld hl,(engineTail)
    ld de,0FF00h
    push hl
    ld hl,(engineTailSize)
    ld b,h
    ld c,l
    pop hl
    jp localCopy

; Блочный контракт: смещения выровнены; проверки выполнены вызывающим тестом.
engineRun:
    ld a,(engineMode)
    cp 2
    jp nc,engineUniversal
    or a
    ld hl,r4Block256
    jp z,engineBlockReady
    ld hl,cpmBlock256
engineBlockReady:
    ld (engineBlockCall+1),hl
    ld b,32
engineBlockLoop:
    push bc
    ld hl,farRequest
engineBlockCall:
    call r4Block256
    pop bc
    ld hl,farRequest+2
    inc (hl)
    ld hl,farRequest+5
    inc (hl)
    dec b
    jp nz,engineBlockLoop
    ; Возвращаем адреса пакета для сравнения полного диапазона.
    ld a,(farRequest+2)
    sub 32
    ld (farRequest+2),a
    ld a,(farRequest+5)
    sub 32
    ld (farRequest+5),a
    xor a
    ret
engineUniversal:
    ld hl,farRequest
    cp 2
    jp z,bankCopyFar
    jp cpmUniversal

; Инверсия назначения относительно источника; все регистры разрушены.
matrixPoison:
    call byteMode
    ld hl,(farRequest+1)
    LOAD_DE farRequest+4
    ld bc,LENGTH
matrixPoisonLoop:
    push bc
    ld a,(farRequest)
    call matrixRead
    ld a,c
    cpl
    ld c,a
    ex de,hl
    ld a,(farRequest+3)
    or a
    jp z,matrixPoisonMain
    dec a
    call 0FFD0h
    jp matrixPoisonNext
matrixPoisonMain:
    ld (hl),c
matrixPoisonNext:
    inc hl
    ex de,hl
    inc hl
    pop bc
    dec bc
    ld a,b
    or c
    jp nz,matrixPoisonLoop
    ret
matrixCompare:
    call byteMode
    ld hl,(farRequest+1)
    LOAD_DE farRequest+4
    ld bc,LENGTH
matrixCompareLoop:
    push bc
    ld a,(farRequest)
    call matrixRead
    ld b,c
    ex de,hl
    ld a,(farRequest+3)
    call matrixRead
    ld a,b
    cp c
    pop bc
    jp nz,matrixMismatch
    inc hl
    ex de,hl
    inc hl
    dec bc
    ld a,b
    or c
    jp nz,matrixCompareLoop
    xor a
    ret
matrixMismatch:
    ld a,1
    ret
; A=логическая страница, HL=адрес. Выход C=байт, AF разрушен.
matrixRead:
    or a
    jp z,matrixReadMain
    dec a
    jp 0FFC0h
matrixReadMain:
    ld c,(hl)
    ret
engineMode: DB 0
engineBank: DB 0
engineStatus: DB 0
engineErrors: DB 0
engineTail: DW 0
engineTailSize: DW 0
engineOutput: DW 0
engineTimings: DS 48,0
engineNames: DW r4BlockName,cpmBlockName,r4UniversalName,cpmUniversalName
