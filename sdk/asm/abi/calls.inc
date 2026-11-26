; MXOS
; Регистровый ABI BIOS и DOS для MAIN, DOS и EXEC.

    IFNDEF MXOS_ABI_CALLS_INC
    DEFINE MXOS_ABI_CALLS_INC

    INCLUDE "memory.inc"
    INCLUDE "syscall-ids.inc"

; Пакет родных групп: порядок совпадает с PUSH AF/BC/DE/HL на 8080.
    STRUCT MX_REGS
hl DW 0
de DW 0
bc DW 0
af DW 0
    ENDS

    ASSERT MX_REGS == 8

; Вызов с регистрами исполнителя. Код операции расположен после CALL.
    MACRO MXOS_CALL_REGS operation
        ASSERT (operation >> 8) == MXOS_GROUP_BIOS || (operation >> 8) == MXOS_GROUP_DOS
        call mx_callRegs
        DW operation
    ENDM
; Вызов с адресом пакета MX_REGS в текущей странице.
    MACRO MXOS_CALL_PACKET operation, packet
        ASSERT (operation >> 8) == MXOS_GROUP_BIOS || (operation >> 8) == MXOS_GROUP_DOS
        ld bc,operation
        ld de,packet
        call mx_callPacket
    ENDM

; Non-returning shell handoff. The explicit MX_REGS and return address live
; on the system stack; mx_callPacket never returns into MAIN code overwritten by
; the loaded program. returnAddress must reset SP (BIOS reboot/EXEC shell).
; Pass ordinary FILE_EXEC registers before this macro; no continuation follows.
    MACRO MXOS_EXEC returnAddress
        push af
        push bc
        push de
        push hl
        ld hl,0
        add hl,sp
        ex de,hl
        ld hl,returnAddress
        push hl
        ld bc,MXOS_DOS_FILE_EXEC
        jp mx_callPacket
    ENDM

    ENDIF
