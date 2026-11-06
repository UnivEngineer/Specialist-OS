; Полная пара переносов для каждой локальной реализации, 8192 байта.
; Вспомогательный код вариантов размещается по 2000h, данные по 5000h.
; Проверка чтения предварительно отравляет main, проверка записи — банк.
ramTests:
    ld hl,blockDesc
    call runPair
    ld hl,byteDesc
    call runPair
    ld hl,variantTable
nextVariant:
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld (tablePointer),hl
    ld a,d
    or e
    jp z,testCPM
    ex de,hl
    call runPair
    ld hl,(tablePointer)
    jp nextVariant
testCPM:
    ld hl,cpmDesc
    jp runPair
; HL — дескриптор: образ, длина, хвост, длина хвоста, имя.
runPair:
    ld (descriptor),hl
    xor a
    ld (readDirection),a
    ; Инверсия старого содержимого исключает ложный успех пустой записи.
    call byteMode
    ld hl,TARGET
    ld de,EXPECTED
    ld bc,LENGTH
poisonBank:
    push bc
    ld a,(de)
    cpl
    ld c,a
    ld a,(targetPage)
    call 0FFD0h
    pop bc
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,poisonBank
    call runOne
    ld a,1
    ld (readDirection),a
    ld hl,EXPECTED
    ld bc,LENGTH
poisonMain:
    ld a,(hl)
    cpl
    ld (hl),a
    inc hl
    dec bc
    ld a,b
    or c
    jp nz,poisonMain
    call runOne
    ; При ошибке чтения восстанавливаем эталон из ROM для следующего теста.
    jp readROM
runOne:
    call installVariant
    call reset_timing
    ld hl,(descriptor)
    ld de,byteDesc
    call bios_cmp_hl_de
    jp z,transferByte
    ld de,cpmDesc
    call bios_cmp_hl_de
    jp z,transferCPM
    ld a,(readDirection)
    or a
    ld hl,0FFD0h
    jp z,setSectorCall
    ld hl,0FFC0h
setSectorCall:
    ld (sectorCall+1),hl
    ld hl,EXPECTED
    ld de,TARGET
    ld b,32
sectorLoop:
    push bc
    push de
    push hl
    ld a,(targetPage)
    ld c,a
sectorCall:
    call 0FFD0h
    pop hl
    pop de
    pop bc
    inc h
    inc d
    dec b
    jp nz,sectorLoop
    jp measured
transferByte:
    ld hl,TARGET
    ld de,EXPECTED
    ld bc,LENGTH
    ld a,(readDirection)
    or a
    jp nz,transferByteRead
    call bankWrite
    jp measured
transferByteRead:
    call bankRead
    jp measured
transferCPM:
    ld hl,EXPECTED
    ld de,TARGET
    ld a,(targetPage)
    ld b,a
    ld c,0FFh
    ld a,(readDirection)
    or a
    jp z,cpmParams
    ex de,hl
    ld c,b
    ld b,0FFh
cpmParams:
    ld (0FFD0h),hl
    ex de,hl
    ld (0FFD2h),hl
    ld hl,LENGTH/16
    ld (0FFD4h),hl
    ld a,c
    ld (0FFD6h),a
    ld a,b
    ld (0FFD7h),a
    ld a,0FFh
    ld (0FFD8h),a
    call 0FF00h
measured:
    call timer_sample
    call verify
    ld (lastOK),a
    or a
    jp z,printRow
    ld hl,ramErrors
    inc (hl)
printRow:
    ld a,(readDirection)
    or a
    jp nz,printReadTime
    ld hl,(descriptor)
    ld de,8
    add hl,de
    call getWord
    ex de,hl
    call puts
    ld hl,pairText
    call puts
    ld hl,v_elapsed
    call print_time_ms
    ld a,(lastOK)
    or a
    ld hl,pairSeparator
    jp z,puts
    ld hl,pairFailedSeparator
    jp puts
printReadTime:
    ld hl,v_elapsed
    call print_time_ms
    ld a,(lastOK)
    jp result
; Установка кода и окна вне измеряемого интервала. Разрушает все регистры.
installVariant:
    call byteMode
    ld hl,(descriptor)
    call getWord
    push de
    call getWord
    ld b,d
    ld c,e
    pop hl
    ld de,02000h
    call localCopy
    ld hl,(descriptor)
    ld de,4
    add hl,de
    call getWord
    push de
    call getWord
    ld b,d
    ld c,e
    pop de
    ld a,b
    or c
    jp z,installWindow
    push bc
    push de
    ld hl,0FF00h
    call bankWrite
    pop hl
    pop bc
    ld de,0FF00h
    call localCopy
installWindow:
    ld hl,02000h
    ld de,0FFC0h
    ld bc,32
    jp localCopy
getWord:
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ret
blockDesc: DW builtinBlockImage,32,0,0,blockName
byteDesc: DW builtinByteImage,32,0,0,byteName
cpmDesc: DW cpmBlob,164,cpmBlob,164,cpmName
descriptor: DW 0
tablePointer: DW 0
readDirection: DB 0
lastOK: DB 0
ramErrors: DB 0
