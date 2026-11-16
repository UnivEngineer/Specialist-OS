; Эксперимент CPM16/32: все параметры продублированы в хвостах страниц.
; Приватный стенд после остановки файловых операций. Не системный драйвер.
; HL -> FAR-пакет. Поддерживаются положительные размеры, кратные CHUNK.
; BC/DE/HL, SP сохранены; окно остаётся COPY до restoreCommand.
; ALIGNED=1: только 256 байт, источник xx00h, счётчик — младший C.
; ALIGNED=2: пакет целых секторов, источник xx00h; C считает байты сектора,
; B увеличивается один раз на сектор, локальный счётчик — число секторов.
    IFNDEF CHUNK
CHUNK = 32
    ENDIF
    IFNDEF ALIGNED
ALIGNED = 0
    ENDIF
    ASSERT CHUNK == 16 || CHUNK == 32
    ORG 07000h
copy:
    push bc
    push de
    push hl
    ld de,request
    ld b,8
    call bytes
    ld hl,0
    add hl,sp
    ld (savedSP),hl
    ; Тестовый контракт проверяется управляющим стендом до запуска.
    ld a,(0FFDFh)
    or a
    jp nz,windowReady
    ld hl,0FFC0h
    ld de,commandShadow
    ld b,32
    call bytes
windowReady:
    ld a,1
    ld (0FFDFh),a
    ld a,(request)
    call pair
    ld (paramsSource),hl
    ld a,(request+3)
    call pair
    ld (paramsDest),hl
    ld a,(callerPage)
    call pair
    ld (paramsReturn),hl
    ld a,h
    ld (cloneReturnValue+1),a
    ld a,l
    ld (cloneReturnPort+1),a
    ld hl,(request+6)
    IF ALIGNED == 2
    ld l,h
    ld h,1
    ELSE
    ld b,4
    IF CHUNK == 32
    inc b
    ENDIF
divide:
    or a
    ld a,h
    rra
    ld h,a
    ld a,l
    rra
    ld l,a
    dec b
    jp nz,divide
    ; Счётчик в хвосте: low, ceil(count/256); low=0 означает 256 порций.
    ld a,l
    or a
    jp z,countReady
    inc h
countReady:
    ENDIF
    ld (cloneCount+1),hl
    ld hl,cloneTemplate
    ld de,0FFC0h
    ld b,cloneEnd-cloneTemplate
    call bytes
    ld a,(request)
    call clone
    ld a,(request+3)
    call clone
    ; Первый выбор источника исполняется ещё в странице инициатора.
    ; Она может быть третьей: ей нужен только source selector, не весь кадр.
    ld hl,(paramsSource)
    ld (tailSource),hl
    ld hl,(request+1)
    ld b,h
    ld c,l
    ld hl,(request+4)
    ex de,hl
    jp tail

; Подготовка обеих локальных копий выполняется CPU и входит в замер.
clone:
    call pair
    ld a,h
    ld (0FFC0h+cloneValue-cloneTemplate+1),a
    ld a,l
    ld (0FFC0h+clonePort-cloneTemplate+1),a
    ld hl,0
    add hl,sp
    ld (0FFC0h+cloneSP-cloneTemplate+1),hl
    ld hl,(paramsReturn)
    ld b,h
    ld c,l
    ld hl,(paramsDest)
    ex de,hl
    ld hl,(paramsSource)
    jp 0FFC0h

cloneTemplate:
cloneValue: ld a,0
clonePort:  ld (0FFFDh),a
    ld sp,tailParams+6
    push bc
    push de
    push hl
cloneCount: ld hl,0
    ld (tailCount),hl
cloneReturnValue: ld a,0
cloneReturnPort:  ld (0FFFCh),a
cloneSP: ld sp,0
    ret
cloneEnd:
    ASSERT cloneEnd-cloneTemplate <= 31

returned:
    ld hl,(savedSP)
    ld sp,hl
    ld a,1
    ld (0FFDFh),a
    pop hl
    pop de
    pop bc
    xor a
    ret
restoreCommand:
    push bc
    push de
    push hl
    ld hl,commandShadow
    ld de,0FFC0h
    ld b,32
    call bytes
    pop hl
    pop de
    pop bc
    ret
bytes:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,bytes
    ret
pair:
    or a
    jp m,romPair
    ld h,a
    ld l,0FCh
    ret z
    dec h
    inc l
    ret
romPair:
    and 15
    ld h,a
    ld l,0FEh
    ret
request: DB 0
    DW 0
    DB 0
    DW 0,0
savedSP: DW 0
callerPage: DB 0
paramsSource: DW 0
paramsDest: DW 0
paramsReturn: DW 0
commandShadow: DS 32,0
frontEnd:
    ASSERT $ < 07800h
tailImage:
    DISP 0FF00h
    DB 0FFh,0FFh,0FFh,0FFh
tail:
    ld hl,(tailSource)
    ld a,h
    ld h,0FFh
    ld (hl),a
    ld h,b
    ld l,c
    ld sp,hl
index = 0
    DUP CHUNK/2
    pop hl
    ld (0FFC0h+index*2),hl
index = index+1
    EDUP
    IF ALIGNED
    ld a,c
    add a,CHUNK
    ld c,a
    ELSE
    ld hl,0
    add hl,sp
    ld b,h
    ld c,l
    ENDIF
    ld hl,(tailDest)
    ld a,h
    ld h,0FFh
    ld (hl),a
    ld hl,CHUNK
    add hl,de
    ld sp,hl
    ex de,hl
index = 0
    DUP CHUNK/2
    ld hl,(0FFC0h+CHUNK-2-index*2)
    push hl
index = index+1
    EDUP
    IF ALIGNED
    jp nz,tail
    IF ALIGNED == 2
    inc b
    ld hl,tailCount
    dec (hl)
    jp nz,tail
    ENDIF
    ELSE
    ld hl,tailCount
    dec (hl)
    jp nz,tail
    inc hl
    dec (hl)
    jp nz,tail
    ENDIF
    ld hl,(tailReturn)
    ld a,h
    ld h,0FFh
    ld (hl),a
    jp returned
tailParams:
tailSource: DW 0
tailDest: DW 0
tailReturn: DW 0
tailCount: DW 0
tailEnd:
    ASSERT $ <= 0FFC0h
    ENT
    END
