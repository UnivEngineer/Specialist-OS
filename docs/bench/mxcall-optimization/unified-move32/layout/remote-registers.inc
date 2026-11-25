; В целевой странице HL/DE/BC уже содержат источник, назначение и длину.
; До установки местного стека SP переносит пару возврата как 16-битный регистр.
bankCopyMoveRemote:
    ld (bankCopySourcePair),hl
    ld hl,0
    add hl,sp
    ld (bankCopyReturnPair),hl
    ld hl,(bankCopySourcePair)
    ld sp,bankCopyStackTop
    call moveCoreCount
    jp bankCopySelectHome
