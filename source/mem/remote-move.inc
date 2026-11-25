; В целевой странице HL/DE/BC уже содержат источник, назначение и длину.
; До установки местного стека SP переносит пару возврата как 16-битный регистр.
bankCopyMoveRemote:
    ld (bankCopySourcePair),hl
    ld hl,0
    add hl,sp
    ld (bankMoveHome+1),hl
    ld hl,(bankCopySourcePair)
    ld sp,bankCopyLocalParams+8
    call moveCoreCount
bankMoveHome:
    ld hl,0
    ld a,h
    ld h,0FFh
    ld (hl),a
    jp bankCopyRemoteDone
