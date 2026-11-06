; Одинаковые публичные входы в исполняемых банках. В main остаётся BIOS.
    INCLUDE "../include/mxos.inc"
    INCLUDE "biosabi.inc"
    INCLUDE "abi.inc"
    INCLUDE "../../bin/banked/transport-imports.inc"
    ORG mx_call
    jp bankMxCall
    jp bankCpmCall
    jp bankMxCallRegs
    jp bankLoadPath
    jp bankSavePath

; B=группа, C=номер, DE=MX_REGS в исходной странице.
; AF/BC/DE/HL — результат исполнителя. SP и страница сохраняются.
; BIOS выполняется в main, файловые функции — в физическом банке ДОЗУ 0.
bankMxCall:
    ld a,b
    cp MX_GROUP_BIOS
    jp z,bankMxCheckPacket
    cp MX_GROUP_DOS
    jp nz,bankMxUnsupported
bankMxCheckPacket:
    ; Пакет не должен пересекать начало системного хвоста.
    ld h,d
    ld l,e
    push bc
    ld bc,MX_REGS
    add hl,bc
    pop bc
    jp c,bankMxBadPacket
    ld a,h
    cp BANK_CLIENT_LIMIT/256
    jp c,bankMxPacketReady
    jp nz,bankMxBadPacket
    ld a,l
    or a
    jp nz,bankMxBadPacket
bankMxPacketReady:
    push de
    ld a,b
    cp MX_GROUP_DOS
    jp z,bankMxDos
    xor a
    ld hl,BANK_BIOS_GATE+3
    jp bankMxInvoke
bankMxDos:
    ld a,(bankPage)
    cp 1
    jp z,bankMxLocalDos
    ld a,1
    ld hl,BANK_DOS_GATE
bankMxInvoke:
    call bankCallPacket
    jp bankMxResult
bankMxLocalDos:
    call BANK_DOS_GATE+3
bankMxResult:
    push af
    push bc
    push de
    push hl
    ld a,(BANK_FRAME_STATUS)
    or a
    jp nz,bankMxReturned
    ld hl,8
    add hl,sp
    ld e,(hl)
    inc hl
    ld d,(hl)
    ld hl,0
    add hl,sp
    ld b,MX_REGS
bankMxSave:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,bankMxSave
bankMxReturned:
    pop hl
    pop de
    pop bc
    pop af
    inc sp
    inc sp
    ret
bankMxBadPacket:
    ld a,DOS_ERR_ARGUMENT
    scf
    ret
bankCpmCall:
    ld b,0
bankMxUnsupported:
    ld a,DOS_ERR_UNSUPPORTED
    scf
    ret

; CALL F806h / DW operation. Пакет расположен на стеке вызывающего банка.
; Ни один регистр не занят номером функции в пользовательском коде.
bankMxCallRegs:
    push af
    push bc
    push de
    push hl
    ld hl,8
    add hl,sp
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld c,(hl)
    inc hl
    ld b,(hl)
    inc hl
    ex de,hl
    ld (hl),d
    dec hl
    ld (hl),e
    ld hl,0
    add hl,sp
    ex de,hl
    call bankMxCall
    inc sp
    inc sp
    inc sp
    inc sp
    inc sp
    inc sp
    inc sp
    inc sp
    ret
bankLoadPath:
    MXCALL MX_DOS_FILELOADPATH
    ret
bankSavePath:
    MXCALL MX_DOS_FILESAVEPATH
    ret
bankClientEnd:
    ASSERT $ <= CMD_LINE
    END
