; Локальный физический вход, отдельный фрагмент устанавливаемого хвоста.
; Находится ниже публичных JMP; одинаковый адрес в main и банке DOS.
    INCLUDE "../include/mxos.inc"
    INCLUDE "abi.inc"
    INCLUDE "../../bin/banked/transport-imports.inc"
    INCLUDE "../../bin/banked/client-imports.inc"
    ORG MEM_IMAGE_LOCAL_START
    INCLUDE "dword.inc"
    INCLUDE "text-route.inc"
    ASSERT $ <= MEM_EXTENSION_ENTRY
    DS MEM_EXTENSION_ENTRY-$,0
    INCLUDE "memory.inc"
    ASSERT $ <= LOCAL_BLOCK_ENTRY
    DS LOCAL_BLOCK_ENTRY-$,0
    INCLUDE "local-block.inc"
    ASSERT $ <= MEM_CALL_ENTRY
    DS MEM_CALL_ENTRY-$,0
; Общий вход: main передаёт пакет локальному диспетчеру BIOS,
; исполняемый банк — общему клиенту. DE/BC сохранены, AF/HL разрушены.
memCall:
    ld a,(bankPage)
    or a
    jp nz,mx_memory+3
    ld hl,(MEM_NATIVE_CALL)
    jp (hl)
    ASSERT $ <= MEM_NATIVE_CALL
    DS MEM_NATIVE_CALL-$,0
memNativeCall: DW 0
memBankKind: DB MEM_BANK_EXEC
memLocalA: DW 0
    ASSERT $ == MEM_LOCAL_INFO_ADDR
; Локальный запрос без DOS и переключения банка. Все регистры результата
; перечислены в mxos.inc; память и профиль окна не изменяются.
memInfo:
    ld a,(bankPageCount)
    ld b,a
    ld a,(MEM_BANK_KIND)
    ld c,a
    ld hl,MEM_ABI_VERSION
    ld de,MEM_CAP_COPY | MEM_CAP_LAZY_WINDOW | MEM_CAP_MEMORY_OPS | MEM_CAP_MODULES | MEM_CAP_ALLOCATOR | MEM_CAP_DWORD
    ld a,(bankPage)
    or a
    ret
    ASSERT $ == MEM_RUNTIME_REVISION_ADDR
memRuntimeRevision: DW MEM_RUNTIME_REVISION
    ASSERT $ == MEM_COMMAND_SHADOW
    DS 32,0
    ASSERT $ == MEM_NEAR_COPY
memNearCopy: DW 0
localBlockEnd:
    ASSERT $ <= mx_call
    END
