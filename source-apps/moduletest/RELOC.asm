; Один модуль для проверки на разных базах и в трёх страницах.
    INCLUDE "../../sdk/asm/mxos.inc"
IMAGE_FLAGS       = TEST_FLAGS
IMAGE_LOAD_OFFSET = 0020h
    ORG IMAGE_LOAD_OFFSET
    IF TEST_HIGH
    RELOCATE_START HIGH
IMAGE_RELOC_KIND = 1
IMAGE_ALIGN_LOG2 = 8
    ELSE
    RELOCATE_START
IMAGE_RELOC_KIND = 0
IMAGE_ALIGN_LOG2 = 0
    ENDIF
body:
    DB 0
;----------------------------------------------------------------------------
; MXOS
; Контроль внутреннего вызова, абсолютного входа MEM и собственного BSS.
; вход: A=MODULE_ACTIVATE, C=0/1 при холодной/тёплой активации. A=0 при вызове.
; В DOS вариант A=1 сохраняет модуль как резидент с ключом 524Ch.
; выход: при активации CF=0, C сохранён в данных. При вызове HL=счётчик,
; DE=адрес BSS, B=номер физической страницы, C=последняя причина активации.
; Флаги и прочие регистры рабочие. Каждый вызов увеличивает счётчик экземпляра.
;----------------------------------------------------------------------------
entry:
    cp MODULE_ACTIVATE
    jp z,activate
    IF TEST_FLAGS == IMAGE_FLAG_DRIVER
    cp 1
    jp z,retain
    ENDIF
    call mx_memInfo
    ld b,a
    call increment
    ex de,hl
    ld a,(lastActivation)
    ld c,a
    ld hl,(counter)
    or a
    ret
    IF TEST_FLAGS == IMAGE_FLAG_DRIVER
retain:
    ld de,0524Ch
    ld a,RESIDENT_RETAIN
    MXOS_CALL_REGS MXOS_DOS_RESIDENT
    ret
    ENDIF
activate:
    ld a,c
    ld (lastActivation),a
    or a
    ret
increment:
    ld hl,(counter)
    inc hl
    ld (counter),hl
    ex de,hl
    ld hl,(workPointer)
    IF TEST_HIGH
    ld h,HIGH workspace
    ENDIF
    ld (hl),e
    inc hl
    ld (hl),d
    dec hl
    ret
lastActivation: DB 0
counter:        DW TEST_SEED
workPointer:    DW workspace
workspace:
    RELOCATE_END
IMAGE_BODY_SIZE    = workspace-body
IMAGE_BSS_SIZE     = 2
IMAGE_MEM_BEGIN    = 0
IMAGE_MEM_END      = IMAGE_LOAD_OFFSET+IMAGE_BODY_SIZE+IMAGE_BSS_SIZE
IMAGE_ENTRY_OFFSET = IMAGE_LOAD_OFFSET+entry-body
IMAGE_CACHEABLE    = TEST_OPTIONS
    RELOCATE_TABLE body
    END
