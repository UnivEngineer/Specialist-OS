; CPM32: публичные проверки уже выполнены, секторный вход доверенный.
    jp bankCopyPrepare32
    jp memBatchIO
bankCopyPrepare32:
    ld a,(BANK_PROFILE)
    or a
    jp z,copySaveCommand
    cp BANK_PROFILE_COUNT
    jp c,copyHaveCommand
copySaveCommand:
    ld hl,BANK_WINDOW
    ld de,MEM_COMMAND_SHADOW
    ld b,8
    call bankCopyQuads
copyHaveCommand:
    ld a,BANK_PROFILE_COPY
    ld (BANK_PROFILE),a
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
    ASSERT $ <= MEM_MAIN_IO_BRIDGE
    DS MEM_MAIN_IO_BRIDGE-$,0
memMainIoBridge:
    jp MAIN_BLOCK_IO_TARGET      ; Генерируется после размещения BIOS.
copyPrepareBody:
    ld a,(bankCopyRequest)
    call bankCopyPair
    ld (bankCopyPairs),hl
    ld a,(bankCopyRequest+3)
    call bankCopyPair
    ld (bankCopyPairs+2),hl
    ld a,(bankPage)
    call bankCopyPair
    ld (bankCopyPairs+4),hl
    ld a,h
    ld (copyCloneReturnValue+1),a
    ld a,l
    ld (copyCloneReturnPort+1),a
    ld hl,(bankCopyRequest+6)
    ld a,l
    and 31
    ld (bankCopyRest),a
    ld a,h
    dec a
    or l
    ld hl,0108h                 ; 256 байт: 8 порций, без деления.
    jp z,copyCountReady
    ld hl,(bankCopyRequest+6)
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
    ld a,h
    or l
    jp z,copyPointers
    ld a,l
    or a
    jp z,copyCountReady
    inc h
copyCountReady:
    ld (copyCloneCount+1),hl
    ld hl,copyCloneTemplate
    ld de,BANK_WINDOW
    ld b,7
    call bankCopyQuads
    ld a,(bankCopyRequest)
    call copyClone
    ld a,(bankCopyRequest+3)
    call copyClone
    ld hl,(bankCopyPairs)        ; Первый выбор источника в третьем инициаторе.
    ld (bankCopySourcePair),hl
copyPointers:
    ld hl,(bankCopyRequest+1)
    ld b,h
    ld c,l
    ld hl,(bankCopyRequest+4)
    ex de,hl
    ld hl,(bankCopyRequest+6)
    ld a,h
    or a
    jp nz,bankCopyTail
    ld a,l
    cp 32
    jp nc,bankCopyTail
    jp bankCopyReturn
copyClone:
    call bankCopyPair
    ld a,h
    ld (BANK_WINDOW+copyCloneValue-copyCloneTemplate+1),a
    ld a,l
    ld (BANK_WINDOW+copyClonePort-copyCloneTemplate+1),a
    ld hl,0
    add hl,sp
    ld (BANK_WINDOW+copyCloneSP-copyCloneTemplate+1),hl
    ld hl,(bankCopyPairs+4)
    ld b,h
    ld c,l
    ld hl,(bankCopyPairs+2)
    ex de,hl
    ld hl,(bankCopyPairs)
    jp BANK_WINDOW
copyCloneTemplate:
copyCloneValue: ld a,0
copyClonePort: ld (0FFFDh),a
    ld sp,bankCopyLocalParams+6
    push bc
    push de
    push hl
copyCloneCount: ld hl,0
    ld (bankCopyCount),hl
copyCloneReturnValue: ld a,0
copyCloneReturnPort: ld (0FFFCh),a
copyCloneSP: ld sp,0
    ret
    DS 2,0
copyCloneEnd:
    ASSERT copyCloneEnd-copyCloneTemplate == 28
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
    ASSERT $ <= MEM_COPY_BYTES_ENTRY
    DS MEM_COPY_BYTES_ENTRY-$,0
    DEFINE COPY_HELPER_BYTES
    INCLUDE "copy-helpers.inc"
    UNDEFINE COPY_HELPER_BYTES
    ASSERT $ <= MEM_COPY_TINY_TEMPLATE
    DS MEM_COPY_TINY_TEMPLATE-$,0
 ; Остаток 1..31: код исполняется в общем окне, без чтения за концом.
bankCopyTinyTemplate:
    ld hl,0
    ld (hl),0
    ld a,(bc)
    inc bc
    ld hl,0
    ld (hl),0
    ld (de),a
    inc de
    ld hl,BANK_WINDOW+29
    dec (hl)
    jp nz,BANK_WINDOW
    ld hl,0
    ld (hl),0
    jp bankCopyOK
    DB 0,0,BANK_PROFILE_COPY
bankCopyTinyEnd:
    ASSERT bankCopyTinyEnd-bankCopyTinyTemplate == 32
    ASSERT $ <= MEM_COPY_QUADS_ENTRY
    DS MEM_COPY_QUADS_ENTRY-$,0
    INCLUDE "copy-helpers.inc"
    ASSERT $ <= MEM_COPY_STATE
    DS MEM_COPY_STATE-$,0
    DS 9,0
