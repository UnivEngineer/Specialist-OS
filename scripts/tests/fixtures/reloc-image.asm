; Исправляемые CALL, JP, DW и BSS, фиксированный BIOS и смещённое начало тела.
    IFNDEF TEST_BASE
TEST_BASE = 0
    ENDIF
IMAGE_LOAD_OFFSET  = 21h
IMAGE_ENTRY_OFFSET = IMAGE_LOAD_OFFSET+1
IMAGE_BSS_SIZE     = 32
    ORG TEST_BASE+IMAGE_LOAD_OFFSET
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
entry:
    call getAddress
    jp z,entry
    call 0C800h
    ret
getAddress:
    ld hl,workspace
    IF TEST_HIGH
    ld h,HIGH workspace
    ENDIF
    ret
    DS 0DBh,0
    dw entry,workspace
workspace:
    RELOCATE_END
IMAGE_BODY_SIZE = workspace-body
IMAGE_MEM_END   = IMAGE_LOAD_OFFSET+IMAGE_BODY_SIZE+IMAGE_BSS_SIZE
    RELOCATE_TABLE body
