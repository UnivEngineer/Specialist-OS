; Опыт для одного вызова в EXEC. Точки подключения и адреса передаёт run.py.
    ORG 06000h
; MAIN: C — число прочитанных байтов. B возвращает исходный C для старой проверки NUL.
aligned:
    push bc
    ld a,c
    cp 32
    jp nz,render
    ld a,(cursorX)
    ld c,a
    ld b,HIGH quota
    ld a,(bc)
    ld c,a
    ld b,0
render:
    ld a,d
    call printWindow
    pop hl
    ld b,l
    ret

; EXEC: как в штатном memPrintTyped, но после двух полных окон переходим на Far256.
hybridStart:
    ld a,2
    ld (windowsLeft),a
    push bc
    push de
    ld a,(operation)
    ld d,a
    jp printLoop
afterWindow:
    ld a,(windowsLeft)
    dec a
    ld (windowsLeft),a
    jp nz,printLoop
    ld (reqSource),hl
    ld a,d
    ld (request),a
    cp 2
    jp z,rawLength
    ; HL на первом непрочитанном байте ASCIIZ. Читаем строго до NUL.
    ld a,(hl)
    or a
    jp z,doneHL
    ld bc,0
scan:
    ld a,(hl)
    inc hl
    inc bc
    or a
    jp nz,scan
rawLength:
    ld a,b
    or c
    jp z,doneHL
    ld h,b
    ld l,c
    ld (reqCount),hl
    ld hl,request
    call callRegs
    DW textPrint
    jp doneHL
windowsLeft: DB 0
request:     DB 0,1
reqSource:   DW 0
reqCount:    DW 0
             DW 0,0
; Для отдельного опыта с обычным LIM первый участок остаётся обычным LIM.
; Получатель уже подменил рендерер только у 256-байтного счётного запроса.
afterWindowLim:
    ld a,(windowsLeft)
    dec a
    ld (windowsLeft),a
    jp nz,printLoop
    ld (reqSource),hl
    ld a,2
    ld (request),a
    jp rawLength
    ASSERT $ < 06100h
quota = 06100h ; Индекс X измеряется в единицах по два пикселя.
