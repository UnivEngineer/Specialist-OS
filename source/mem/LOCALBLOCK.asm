; Локальный физический вход, отдельный фрагмент устанавливаемого хвоста.
; Находится ниже публичных JMP; одинаковый адрес в main и банке DOS.
    INCLUDE "../include/mxos.inc"
    INCLUDE "abi.inc"
    INCLUDE "../../bin/banked/transport-imports.inc"
    ORG LOCAL_BLOCK_ENTRY
    INCLUDE "local-block.inc"
    ASSERT $ <= MEM_CALL_ENTRY
    DS MEM_CALL_ENTRY-$,0
; Общий вход: main передаёт пакет локальному диспетчеру BIOS,
; исполняемый банк — общему клиенту. DE/BC сохранены, AF/HL разрушены.
memCall:
    ld a,(bankPage)
    or a
    jp nz,mx_call+15
    ld hl,(MEM_NATIVE_CALL)
    jp (hl)
    ASSERT $ <= MEM_NATIVE_CALL
    DS MEM_NATIVE_CALL-$,0
memNativeCall: DW 0
memBankKind: DB MEM_BANK_EXEC
memLocalA: DW 0
localBlockEnd:
    ASSERT $ <= mx_call
    END
