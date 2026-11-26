    INCLUDE "../../../sdk/asm/mxos.inc"
    ORG 0000h

;----------------------------------------------------------------------------
; MXOS
; Запросить все DATA-банки целиком, как это делает приложение с большой моделью.
; вход: MAIN, аргументы игнорируются.
; выход: на метке probeResult можно прочитать число выданных банков и
; пакет последнего отказа. Блоки остаются у приложения до RET.
;----------------------------------------------------------------------------
probeEntry:
    xor a
    ld (probeBanks),a
probeAllocate:
    ld hl,MEM_DATA_LIMIT
    ld b,MEM_ALLOC_ANY
    ld a,MEM_OP_ALLOC
    MXOS_CALL_REGS MXOS_DOS_MEMORY
    jp c,probeFailed
    ld hl,probeBanks
    inc (hl)
    jp probeAllocate
probeFailed:
    ld (probeName),hl
    ex de,hl
    ld (probeReason),hl
    ld h,b
    ld l,c
    ld (probeDetail),hl
    ld (probeStatus),a
probeResult:
    ret
probeBanks:  DB 0
probeStatus: DB 0
probeDetail: DW 0
probeName:   DW 0
probeReason: DW 0
