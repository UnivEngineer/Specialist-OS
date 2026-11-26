    INCLUDE "../../source/common/system.inc"

    IFNDEF FONTGRID_ORIGIN
        DEFINE FONTGRID_ORIGIN 0
    ENDIF
    ORG FONTGRID_ORIGIN
    RELOCATE_START HIGH
;----------------------------------------------------------------------------
; MXOS
; Измерить банковый маршрут FontBen в текущей странице EXEC.
; вход:
;   HL = адрес GRID_TEXT_BYTES+1 байтов корпуса в MAIN, DE = начало рабочей
;   арены MAIN для Far1024. C = номер API 0..4, B = способ вызова 0 или 1.
;   Таймер захвачен вызывающим FontBen до входа в DLL.
; выход:
;   CF=0, DE:HL = число тиков ВИ53. При отказе CF=1, A = код ошибки.
;   Остальные регистры и флаги разрушаются. Корпус живёт до выгрузки DLL.
;----------------------------------------------------------------------------
entry:
    ld (gridCorpusCopy+1),hl
    ex de,hl
    ld (gridScratch),hl
    push bc
    call mx_memInfo
    ld (gridCorpusCopy+3),a
    ld h,HIGH gridCorpus
    ld l,0
    ld (gridCorpusCopy+4),hl
    ld hl,gridCorpusCopy
    call mx_memcpyFar
    pop bc
    ret c
    jp gridBank

gridScratch:    DW 0
gridCorpusCopy: DB 0
                DW 0
                DB 0
                DW 0,GRID_TEXT_BYTES+1
    DEFINE GRID_BANK_CORPUS gridCorpus
    DEFINE GRID_BANK_SCRATCH gridScratch
    INCLUDE "grid-bank.inc"
    INCLUDE "../textbench/module-timing.inc"
gridFileEnd:
gridCorpus          = entry+((gridFileEnd-entry+255) & 0FF00h)
    RELOCATE_END
IMAGE_FLAGS         = IMAGE_FLAG_BANK
IMAGE_BODY_SIZE     = gridFileEnd-entry
IMAGE_RELOC_KIND    = 1
IMAGE_ALIGN_LOG2    = 8
IMAGE_MEM_BEGIN     = 0
IMAGE_MEM_END       = gridCorpus-entry+GRID_TEXT_BYTES+1
IMAGE_BSS_SIZE      = IMAGE_MEM_END-IMAGE_BODY_SIZE
IMAGE_MEMORY_POLICY = IMAGE_POLICY_PRESERVE
IMAGE_API           = 2
    RELOCATE_TABLE entry
