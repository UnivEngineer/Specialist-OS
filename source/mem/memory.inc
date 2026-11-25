; Общие операции памяти; одинаковое тело в main и исполняемых банках.
; A=операция; MOVE: HL=MEM_COPY_REQUEST. GET/PUT: B=банк, HL=адрес,
; DE=значение (для байта E). GET возвращает DE; BC/HL сохранены.
; PRINT: HL=локальная ASCIIZ, возвращает HL на нуле, BC/DE сохранены.
    jp memMemory
    jp memCopySelect
    jp memBiosRoute
    jp bankMxResult
    jp memMovePrepared
    jp memCommandScalarReturn
    jp memCopyThere
memMemory:
    call bankQueueBefore
    or a
    jp z,memMove
    cp MEM_OP_PRINT
    jp z,memPrint
    cp MEM_OP_PRINT+1
    jp nc,memBad
    jp memScalar
memBad:
    ld a,MEM_ERROR_RANGE
    scf
    ret

; Оба публичных входа после сброса очереди и общих проверок используют выбор по паре страниц.
memMove = bankCopyEnter
memMovePrepared:
memCopySelect:
    ld a,(bankCopyRequest)
    ld b,a
    ld a,(bankCopyRequest+3)
    cp b
    jp nz,bankCopyPrepared
    ld a,(bankPage)
    cp b
    jp nz,memCopyMoveRemote
    ld hl,bankCopyRequest
    call memNativeCopy
    jp bankCopyOK
    INCLUDE "remote-move-prepare.inc"
memCopyThere = memCopySelect
memExecCopy = bankUnifiedCore
;----------------------------------------------------------------------------
; MXOS
;
; Обмен порцией между DOS и окном в контексте исполняемого клиента.
; вход:
;   A = страница клиента, B = проверенная длина 1..32, C = порог завершения
;   (0 для бинарных данных, 1 для ASCIIZ, 20h для командной строки).
;   Чтение берёт источник из HL, запись берёт приёмник из DE и требует C=0.
; выход:
;   Выбрана DOS, HL и DE за последним байтом источника и приёмника, C сохранён.
;   CF=1 при переданном байте ниже порога. Стек используется только в DOS.
;----------------------------------------------------------------------------
memDosReadChunk:
    ld de,BANK_WINDOW
    jp memDosCopyMap
memDosWriteChunk:
    ld hl,BANK_WINDOW
    jp memDosCopyMap
    ASSERT $ <= 0F475h
    DS 0F475h-$,0
    INCBIN "../../bin/banked/fast.bin"
    ASSERT $ <= 0F4FFh
    DS 0F4FFh-$,0

memScalar:
    push bc
    push hl
    push af
    push hl
    push de
    dec a
    ld e,a
    ld d,0
    ld hl,memScalarKinds
    add hl,de
    ld a,(hl)
    ld (BANK_SCALAR_KIND),a
    ld a,(bankPage)
    call bankCopyPair
    ld (BANK_SCALAR_RETURN_PAIR),hl
    ld a,b
    call bankCopyPair
    ld a,h
    ld h,0FFh
    pop bc
    pop de
    jp BANK_SELECT_GATE
memCommandScalarReturn:
    ld d,b
    ld e,c
    pop af
    cp MEM_OP_GET8
    jp nz,memScalarDone
    ld d,0
memScalarDone:
    xor a
    pop hl
    pop bc
    ret
memScalarKinds:     DB 080h,0C0h,04h,040h

memRouteBad:
    ld a,MEM_ERROR_HANDLE
    scf
    ret
memDosCopyMap:
    dec a
    jp m,memDosCopyMain
    ld (BANK_IO_RAM),a
    jp memDosCopyMapped
memDosCopyMain:
    ld (BANK_IO_MAIN),a
memDosCopyMapped:
    MEM_TEXT_FILL memDosCopyByte, memDosCopyDone
memDosCopyDone:
    ld a,DOS_BANK_PAGE-1
    ld (BANK_IO_RAM),a
    ret
memRoute = MEM_ROUTE_STATE
memExtensionEnd:
