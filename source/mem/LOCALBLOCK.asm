; Общая сборка LOCAL/CLIENT/TEXT с взаимными ссылками.
; Сборщик разделяет бинарник по mx_call на два установочных сегмента.
    INCLUDE "../include/mxos.inc"
    INCLUDE "biosabi.inc"
    INCLUDE "abi.inc"
    INCLUDE "../../bin/banked/transport-imports.inc"
    INCLUDE "../../bin/banked/main-io-import.inc"
    ORG MEM_IMAGE_LOCAL_START
    INCLUDE "copy-prepare.inc"
    INCLUDE "text-route.inc"
; Verify the scoped handle locally. Carry reports invalid handle or DLL AF.
memRouteChecked:
    push de
    ex de,hl
    ld hl,(memRoute)
    ld a,l
    xor e
    ld l,a
    ld a,h
    xor d
    or l
    pop de
    jp nz,memRouteBad
    ld hl,(memRoute+2)
    ld b,h
    ld c,l
    ld a,(bankHome)
    push af
    ld a,(memRoute+5)
    ld (bankHome),a
    ld a,(memRoute+4)
    ld hl,bankModuleCallback
    jp memRouteInvoke
    ASSERT $ <= MEM_EXTENSION_ENTRY
    DS MEM_EXTENSION_ENTRY-$,0
    INCLUDE "memory.inc"
    ASSERT $ <= LOCAL_BLOCK_ENTRY
    DS LOCAL_BLOCK_ENTRY-$,0
    INCLUDE "local-block.inc"
    ASSERT $ <= MEM_CALL_ENTRY
    DS MEM_CALL_ENTRY-$,0
;----------------------------------------------------------------------------
; MXOS
;
; Выполнить пакетный системный вызов mx_call после сброса командной очереди.
;
; вход:
;   b = группа, c = номер функции, de = адрес изменяемого MX_REGS в текущем
;   банке. Для MAIN используется MEM_NATIVE_CALL, для банка — bankMxCall.
;
; выход:
;   AF, BC, DE и HL содержат результат исполнителя. Пакет получает тот же
;   результат при выполненном вызове. При отказе транспорта до исполнения
;   cf = 1, пакет сохраняется. Стек и банк вызывающего восстанавливаются.
;----------------------------------------------------------------------------
memCall:
    call bankQueueBefore
    ld a,(bankPage)
    or a
    jp nz,bankMxCall
    ld hl,(MEM_NATIVE_CALL)
    jp (hl)
    ASSERT $ <= MEM_NATIVE_CALL
    DS MEM_NATIVE_CALL-$,0
memNativeCall:  DW 0
memBankKind:    DB MEM_BANK_EXEC
memLocalA:      DW 0
    ASSERT $ == MEM_LOCAL_INFO_ADDR
;----------------------------------------------------------------------------
; MXOS
;
; Запросить локальные параметры полного профиля MEM через mx_memInfo.
;
; вход:
;   Параметров нет.
;
; выход:
;   hl = MEM_ABI_VERSION, de = маска MEM_CAP_*, a = FAR-код текущего банка,
;   b = число банков ДОЗУ, c = назначение текущего банка (MEM_BANK_*).
;   cf = 0. DOS не вызывается, системная память и профиль окна сохраняются.
;----------------------------------------------------------------------------
memInfo:
    ld a,(bankPageCount)
    ld b,a
    ld a,(MEM_BANK_KIND)
    ld c,a
    ld hl,MEM_ABI_VERSION
    ld de,MEM_CAP_COPY | MEM_CAP_LAZY_WINDOW | MEM_CAP_MEMORY_OPS | MEM_CAP_MODULES | MEM_CAP_ALLOCATOR | MEM_CAP_COMMAND_QUEUE
    ld a,(bankPage)
    or a
    ret
    ASSERT $ == MEM_RUNTIME_REVISION_ADDR
memRuntimeRevision: DW MEM_RUNTIME_REVISION
    ASSERT $ == MEM_COMMAND_SHADOW
    DS 32,0
    ASSERT $ == MEM_NEAR_COPY
memNearCopy: DW 0
    INCLUDE "regs-route.inc"
mainLocalEnd:
    ASSERT $ <= MEM_IO_SYSTEM
    INCLUDE "local-io.inc"
; Only the EXEC/DOS profile takes this branch. MAIN dispatches before it
; and may use this region for its physical A: driver.
bankMxRegsDos:
    ld a,(bankPage)
    cp DOS_BANK_PAGE
    jp z,BANK_DOS_GATE+3
    ld a,DOS_BANK_PAGE
    ld hl,BANK_DOS_GATE
    jp bankCallPacket
localBlockEnd:
    ASSERT $ <= mx_call
    DS mx_call-$,0
    INCLUDE "CLIENT.asm"
    END
