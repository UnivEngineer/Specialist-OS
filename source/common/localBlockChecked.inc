; B=5, C=диск, DE=блок, HL=локальный сектор. Проверка в банке клиента.
; BANK: не менее 320 свободных байт стека; DOS: DOS_VERIFY_BUFFER.
; MAIN: общий временный буфер BIOS. BC/DE/HL сохраняются.
localBlockChecked:
    push bc
    ld a,c
    ld (lbcDrive),a
    ld bc,lbcCall
    call diskWriteChecked
    pop bc
    ret
lbcCall:
    ld a,(lbcDrive)
    ld c,a
    jp LOCAL_BLOCK_ENTRY
lbcDrive: DB 0
