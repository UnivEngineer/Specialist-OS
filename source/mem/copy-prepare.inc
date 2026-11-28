; CPM32: публичные проверки уже выполнены, секторный вход доверенный.
    jp bankCopyPrepare32
    jp memBatchIO
bankCopyPrepare32:
    ld hl,0
    add hl,sp
    ld (bankCopySP),hl
    jp copyPrepareBody
; A=банк пакета, DE=пакет, BC=его длина (6+2*N). Доверенный вход DOS.
; Пакет: C=диск/B=операция, адрес буфера, N:word, N слов LBA.
; На выходе BC=число успешно переданных секторов, CF/A=статус.
memBatchIO:
    ld (localRamCopy),a
    ld h,b
    ld l,c
    ld (localRamCopy+6),hl
    ex de,hl
    ld (localRamCopy+1),hl
    ld hl,0
    add hl,sp
    ld (memBatchRestore+1),hl
    ld hl,-134
    add hl,sp
    ld sp,hl
    ld (localRamCopy+4),hl
    ld a,(bankPage)
    ld (localRamCopy+3),a
    ld hl,localRamCopy
    call bankCopyFar
    ld hl,256
    ld (localRamCopy+6),hl
    jp c,memBatchFailedCopy
    pop bc
    pop hl
    pop de
    ld a,e
    ld (memBatchRemaining+1),a
    ld (memBatchTotal+1),a
    jp memBatchNext
memRouteCall:
    call bankQueueBefore
    ld a,h
    or l
    jp z,memRouteBad
    jp memRouteChecked
memRouteInvoke:
    call bankCallPacket
    ex (sp),hl
    push af
    ld a,h
    ld (bankHome),a
    pop af
    ex (sp),hl
    inc sp
    inc sp
    ret
    MEM_CURSOR_GATE memFixedSetCursor,0
    ASSERT $ <= MEM_MAIN_IO_BRIDGE
memPrintZEmpty:
    pop de
    pop hl
    pop bc
    jp memPrintHaveHL
    DS MEM_MAIN_IO_BRIDGE-$,0
memMainIoBridge:
    jp MAIN_BLOCK_IO_TARGET      ; Генерируется после размещения BIOS.
copyPrepareBody:
    ld a,(bankCopyRequest)
    call bankCopyPair
    ld (BANK_COPY_SETUP_SOURCE),hl
    ld a,(bankCopyRequest+3)
    call bankCopyPair
    ld (BANK_COPY_SETUP_DEST),hl
    ld a,(bankPage)
    call bankCopyPair
    ld (BANK_COPY_SETUP_HOME),hl
    ld hl,(bankCopyRequest+6)
    ld a,l
    and 31
    ld (bankCopyRest),a
    ld a,h
    dec a
    or l
    ld hl,0108h                 ; 256 байт: 8 окон, без остатка.
    jp z,copyCountReady
    ld hl,(bankCopyRequest+6)
    ld a,h
    or a
    jp nz,copyDivideStart
    ld a,l
    cp 32
    ld hl,0
    jp c,copyCountReady
    ld l,a
copyDivideStart:
    ld b,5
copyDivide32:
    or a
    ld a,h
    rra
    ld h,a
    ld a,l
    rra
    ld l,a
    dec b
    jp nz,copyDivide32
    ld a,l
    or a
    jp z,copyCountReady
    inc h
copyCountReady:
    ld (BANK_COPY_SETUP_COUNT),hl
    ld hl,-2                    ; CALL copyClone кладёт адрес возврата по этому SP.
    add hl,sp
    ld (BANK_COPY_SETUP_SP),hl
    ld hl,(BANK_COPY_SETUP_SOURCE)
    call copyClone
    ld hl,(BANK_COPY_SETUP_DEST)
    call copyClone
    ld hl,(BANK_COPY_SETUP_SOURCE)
    ld (bankCopySourcePair),hl   ; Первый выбор источника из третьего банка.
    ld hl,(bankCopyRequest+1)
    ld b,h
    ld c,l
    ld hl,(bankCopyRequest+4)
    ex de,hl
    ld hl,(bankCopyRequest+6)
    ld a,h
    or a
    jp nz,bankCopyTail
    ; Для физического входа Z сброшен и при размере ровно 32.
    ld a,31
    cp l
    jp c,bankCopyTail
    jp bankCopyReturn
copyClone:
    ld a,h
    ld h,0FFh
    jp bankCopyClone
copyPrepareEnd:
; SP идёт по уже локальному списку. CALL использует только прочитанную часть.
memBatchNext:
    pop de
    push hl
    push bc
    call LOCAL_BLOCK_ENTRY
    pop bc
    pop hl
    jp c,memBatchResult
    inc h
memBatchRemaining:
    ld a,0
    dec a
    ld (memBatchRemaining+1),a
    jp nz,memBatchNext
    xor a
memBatchResult:
    push af
memBatchTotal:
    ld a,0
    ld hl,memBatchRemaining+1
    sub (hl)
    ld c,a
    ld b,0
    pop af
memBatchRestore:
    ld sp,0
    ret
memBatchFailedCopy:
    ld bc,0
    jp memBatchRestore
    MEM_CURSOR_GATE memFixedGetCursor,1
    ASSERT $ <= MEM_QUEUE_WORK_ENTRY
    DS MEM_QUEUE_WORK_ENTRY-$,0
    INCLUDE "queue-work.inc"
    INCLUDE "fixed-word-return.inc"
    ASSERT $ <= MEM_COPY_BYTES_ENTRY
memOperation: DB 0
    DS MEM_COPY_BYTES_ENTRY-$,0
    DEFINE COPY_HELPER_BYTES
    INCLUDE "copy-helpers.inc"
    UNDEFINE COPY_HELPER_BYTES
    INCLUDE "near-copy.inc"
memFixedHexWord:
    push de
    ld de,BIOS_BANK_PRINT_HEX_WORD
    jp memFixedEnter
    ASSERT $ <= MEM_COPY_QUADS_ENTRY
    DS MEM_COPY_QUADS_ENTRY-$,0
    INCLUDE "copy-helpers.inc"
    ASSERT $ <= MEM_COPY_STATE
    DS MEM_COPY_STATE-$,0
    DS 3,0
