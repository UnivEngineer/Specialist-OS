; FILE_PATH_IO сохраняет публичный контракт: count — вместимость при чтении
; или точная длина при записи; actual=0 при ошибке. Структура и путь локальны,
; данные переносит общий fatStream через kernel-stream.inc.
kdPathIO:
    ld hl,(kdRegs)
    ld (kdClientObject),hl
    ld de,KD_OBJECT
    ld bc,FILE_PATH_IO
    call kdFetch
    jp c,kdBad
    ld hl,0
    ld (KD_OBJECT+6),hl
    ld hl,(KD_OBJECT)
    ld (kpPath),hl
    ld de,KD_PATH
    ld bc,MAX_PATH
    call kdFetch
    jp c,kpBad
    ld hl,KD_PATH
    ld b,MAX_PATH
kpScan:
    ld a,(hl)
    or a
    jp z,kpValidate
    inc hl
    dec b
    jp nz,kpScan
    jp kpBad
kpValidate:
    ld hl,(KD_OBJECT+2)
    ld (kpBuffer),hl
    ld (kdFarCurrent),hl
    ld hl,(KD_OBJECT+4)
    ld (pathIoCount),hl
    ld b,h
    ld c,l
    ld hl,(kpBuffer)
    call kdRange
    jp c,kpBad
    ld hl,(kdClientObject)
    ex de,hl
    ld hl,(kpBuffer)
    ld a,FILE_PATH_IO
    call kdOverlap
    jp c,kpBad
    ld hl,(kpPath)
    ex de,hl
    ld hl,(kpBuffer)
    ld a,MAX_PATH
    call kdOverlap
    jp c,kpBad
    ; Путь также не должен пересекать рабочий дескриптор результата.
    ld hl,(kdClientObject)
    ex de,hl
    ld hl,(kpPath)
    ld bc,MAX_PATH
    ld a,FILE_PATH_IO
    call kdOverlap
    jp c,kpBad
    ld hl,KD_PATH
    ld (KD_OBJECT),hl
    ld hl,KD_DATA
    ld (KD_OBJECT+2),hl
    ld (pathIoBuffer),hl
    ld hl,KD_OBJECT
    ld (pathIoRequest),hl
    ld a,(kdFunction)
    sub MXOS_DOS_FILE_LOAD_PATH & 0FFh
    ld (pathIoWrite),a
    ld a,1
    ld (kdFarActive),a
    ; Диапазоны уже проверены выше, дальнейший алгоритм полностью общий.
    call pathIoRangeReady
    push af
    xor a
    ld (kdFarActive),a
    ld hl,(kpPath)
    ld (KD_OBJECT),hl
    ld hl,(kpBuffer)
    ld (KD_OBJECT+2),hl
    pop af
    jp kpReturn
kpBad:
    call blockArgument
kpReturn:
    push af
    pop hl
    ld (kdRegs+6),hl
    ld hl,(kdClientObject)
    ex de,hl
    ld hl,KD_OBJECT
    ld bc,FILE_PATH_IO
    call kdStore
    jp c,kdBad
    jp kdRestorePointer

; [HL,HL+BC) против [DE,DE+A), A=1..255. Диапазоны проверены отдельно.
; BC сохранён; AF/DE/HL разрушены. CF=1 при перекрытии, пустой буфер допустим.
kdOverlap:
    push af
    ld a,b
    or c
    jp z,koGood
    push hl
    add hl,bc
    call cmp_hl_de
    pop hl
    jp c,koGood
    jp z,koGood
    pop af
    push hl
    ld h,0
    ld l,a
    add hl,de
    ex de,hl
    pop hl
    jp cmp_hl_de
koGood:
    pop af
    or a
    ret
kpPath: DW 0
kpBuffer: DW 0
