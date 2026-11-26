; Общая сборка LOCAL/CLIENT/TEXT с взаимными ссылками.
; Сборщик разделяет бинарник по mx_callPacket на два установочных сегмента.
    INCLUDE "../common/system.inc"
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
bankCpmCall:
bankMxUnsupported:
    ld a,DOS_ERR_UNSUPPORTED
    scf
    ret
    ASSERT $ <= MEM_EXTENSION_ENTRY
    DS MEM_EXTENSION_ENTRY-$,0
    INCLUDE "memory.inc"
    INCLUDE "fixed-main.inc"
    INCLUDE "fixed-top.inc"
    ASSERT $ <= LOCAL_BLOCK_ENTRY
    DS LOCAL_BLOCK_ENTRY-$,0
    INCLUDE "local-block.inc"
    ASSERT $ <= MEM_CALL_PACKET_ENTRY
    DS MEM_CALL_PACKET_ENTRY-$,0
;----------------------------------------------------------------------------
; MXOS
;
; Выполнить пакетный системный вызов mx_callPacket после сброса командной очереди.
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
memCallPacket:
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
;   cf = 0. DOS не вызывается, системная память и окно сохраняются.
;----------------------------------------------------------------------------
memInfo:
    ld a,(bankPageCount)
    ld b,a
    ld a,(MEM_BANK_KIND)
    ld c,a
    ld hl,MEM_ABI_VERSION
    ld de,MEM_CAP_COPY | MEM_CAP_MEMORY_OPS | MEM_CAP_MODULES | MEM_CAP_ALLOCATOR | MEM_CAP_COMMAND_QUEUE | MEM_CAP_BIOS_REGISTERS
    ld a,(bankPage)
    or a
    ret
    ASSERT $ == MEM_RUNTIME_REVISION_ADDR
memRuntimeRevision: DW MEM_RUNTIME_REVISION
    INCLUDE "fixed-enter.inc"
    ASSERT $ <= MEM_NEAR_COPY
    DS MEM_NEAR_COPY-$,0
    ASSERT $ == MEM_NEAR_COPY
memNearCopy: DW 0
    INCLUDE "regs-route.inc"
mainLocalEnd:
    ASSERT $ <= MEM_IO_SYSTEM
    INCLUDE "local-io.inc"
;----------------------------------------------------------------------------
; MXOS
; Распознать регистровую BIOS-службу без вложенных вызовов.
; вход: DOS/EXEC, стек содержит AF и адрес DW после CALL F806h; BC/DE/HL исходные.
; выход: обычная ветвь восстанавливает кадр декодера; быстрая возвращает AF/BC/DE/HL за DW.
;----------------------------------------------------------------------------
bankFastDecode:
    pop af
    ex (sp),hl
    push af
    inc hl
    ld a,(hl)
    dec hl
    dec a
    jp nz,bankFastCheckDos
    ld a,(hl)
    cp MXOS_BIOS_UPPERCASE & 255
    jp z,bankFastReady
    cp MXOS_BIOS_SUB_HL_DE & 255
    jp z,bankFastReady
    jp bankFastCheckText
bankFastCheckDos:
    dec a
    jp nz,bankFastFallback
    ld a,(hl)
    cp MXOS_DOS_GET_ERROR & 255
    jp z,bankFastDos
bankFastFallback:
    pop af
    ex (sp),hl
    push af
    jp bankMxCallRegsSaved
; В MAIN пакетный вызов обслуживает нативная таблица BIOS.
bankMxCall:
    ld a,b
    dec a
    jp nz,bankMxDos
    push de
    jp memBiosRoute
localBlockEnd:
    ASSERT $ <= mx_callPacket
    DS mx_callPacket-$,0
    INCLUDE "CLIENT.asm"
    END
