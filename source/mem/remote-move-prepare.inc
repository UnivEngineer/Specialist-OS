; Хвостовой переход из memCopySelect после проверки полного пакета.
; Вход: bankCopyRequest содержит ненулевой перенос внутри одной удалённой страницы.
; Выход: bankCopyRemoteDone восстанавливает SP инициатора и регистры публичного ABI.
memCopyMoveRemote:
    ld hl,0
    add hl,sp
    ld (bankCopySP),hl
    ld a,(bankPage)
    call bankCopyPair
    ld sp,hl
    ld hl,(bankCopyRequest+6)
    ld b,h
    ld c,l
    ld hl,(bankCopyRequest+4)
    ex de,hl
    ld hl,(bankCopyRequest+1)
    ld a,(bankCopyRequest)
    or a
    jp z,bankMoveToMain
    dec a
    jp bankMoveToRam
