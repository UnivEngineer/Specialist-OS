; Линейное системное ПЗУ MX. Загрузчик дискеты устанавливает этот вход в MAIN.
    INCLUDE "../../bin/mx/imports.inc"
    ORG MX_ROM_SLOT
    jp mxRomBlock
; Читаем число секторов BPB из ПЗУ. До возврата восстанавливаем MAIN со стеком.
mxRomProbe:
    xor a
    ld (0FFFEh),a
    ld hl,(3)
    ld de,0584Dh             ; OEM «MXOS256 »
    ld a,l
    cp e
    jp nz,mxRomAbsent
    ld a,h
    cp d
    jp nz,mxRomAbsent
    ld hl,(19)
    ld a,h
    or a
    jp nz,mxRomAbsent
    ld a,l
    cp 128
    jp z,mxRomProbed
    cp 192
    jp z,mxRomProbed
mxRomAbsent:
    ld hl,0
mxRomProbed:
    ld (0FFFCh),a
    ld (MX_ROM_BLOCKS),hl
    ret

mxRomBlock:
    ld a,b
    cp 3
    jp z,mxRomSize
    cp 0FFh
    jp z,mxRomInfo
    cp 4
    ret z
    cp 2
    jp z,mxRomRead
    ld a,DOS_ERR_WRITE_PROTECTED
    scf
    ret
mxRomSize:
    ld hl,(MX_ROM_BLOCKS)
    ex de,hl
    ld bc,0
    jp mxRomQueryDone
mxRomInfo:
    ld bc,MX_ROM_CAPS
mxRomQueryDone:
    ld hl,MX_ROM_INFO
    xor a
    ret
mxRomRead:
    ld a,d
    or a
    jp nz,mxRomRange
    ld a,(MX_ROM_BLOCKS)
    cp e
    jp z,mxRomRange
    jp c,mxRomRange
    ld a,e
    ld (mxRomCopy+2),a
    ld (mxRomCopy+4),hl
    push hl
    ld hl,mxRomCopy
    call MX_COPY_SECTOR
    pop hl
    ret
mxRomRange:
    ld a,DOS_ERR_ARGUMENT
    scf
    ret
mxRomCopy:
    DB 080h
    DW 0
    DB 0
    DW 0,256
    ASSERT $ <= MX_ROM_SLOT_END
    END
